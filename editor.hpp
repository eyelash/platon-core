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
	constexpr Range get_range() const {
		return is_reversed() ? Range(head, tail) : Range(tail, head);
	}
};

class Selections: public std::vector<Selection> {
public:
	std::size_t last_selection;
	Selections(): last_selection(0) {
		emplace_back(0);
	}
	Selection& get_last_selection() {
		return operator [](last_selection);
	}
	std::size_t lower_bound(std::size_t cursor) const {
		std::size_t i;
		for (i = 0; i < size(); ++i) {
			const Selection& selection = operator [](i);
			if (selection.max() >= cursor) {
				break;
			}
		}
		return i;
	}
	void collapse(bool reverse_direction) {
		for (std::size_t i = 1; i < size(); ++i) {
			Selection& lhs = operator [](i - 1);
			Selection& rhs = operator [](i);
			if (lhs.head == rhs.head || lhs.max() > rhs.min()) {
				if (reverse_direction) {
					lhs = Selection(rhs.max(), lhs.min());
				}
				else {
					lhs = Selection(lhs.min(), rhs.max());
				}
				if (last_selection >= i) {
					--last_selection;
				}
				erase(begin() + i);
				--i;
			}
		}
	}
	template <class... A> void set_selection(A&&... a) {
		clear();
		emplace_back(std::forward<A>(a)...);
		last_selection = 0;
	}
};

struct RenderedLine {
	std::string text;
	std::size_t number;
	std::vector<Span> spans;
	std::vector<Range> selections;
	std::vector<std::size_t> cursors;
};

constexpr Range operator -(const Range& range, std::size_t pos) {
	return Range(range.start - pos, range.end - pos);
}

class Editor {
	TextBuffer buffer;
	const Language* language;
	mutable Cache cache;
	Selections selections;
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
			const Range intersection = selection.get_range() & Range(index0, index1);
			if (intersection) {
				line.selections.emplace_back(intersection - index0);
			}
			if (selection.head >= index0 && selection.head < index1) {
				line.cursors.emplace_back(selection.head - index0);
			}
		}
	}
	static const char* get_file_name(const char* path) {
		const char* file_name = path;
		for (const char* i = path; *i != '\0'; ++i) {
			if (Path::is_separator(*i)) {
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
	class SelectionIterator {
	public:
		Editor* editor;
		std::size_t insertion_offset = 0;
		std::size_t deletion_offset = 0;
		std::size_t i = 0;
		constexpr SelectionIterator(Editor* editor): editor(editor) {}
		constexpr bool operator <(std::size_t rhs) const {
			return i < rhs;
		}
		Selection& operator *() const {
			return editor->selections[i];
		}
		Selection* operator ->() const {
			return &editor->selections[i];
		}
		SelectionIterator& operator ++() {
			++i;
			return *this;
		}
		void delete_text() {
			Selection& selection = editor->selections[i];
			if (!selection.is_empty()) {
				editor->cache.invalidate(selection.min());
				for (std::size_t i = selection.min(); i < selection.max(); ++i) {
					editor->buffer.remove(selection.min());
					++deletion_offset;
				}
				selection = selection.min();
			}
		}
		void insert(char c) {
			Selection& selection = editor->selections[i];
			editor->cache.invalidate(selection.head);
			editor->buffer.insert(selection.head, c);
			selection += 1;
			++insertion_offset;
		}
		void insert(const char* text) {
			Selection& selection = editor->selections[i];
			editor->cache.invalidate(selection.head);
			for (const char* c = text; *c; ++c) {
				editor->buffer.insert(selection.head, *c);
				selection += 1;
				++insertion_offset;
			}
		}
	};
public:
	Editor(): language(nullptr) {}
	Editor(const char* path): buffer(path), language(prism::get_language(get_file_name(path))) {}
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
	template <class F> void for_each_selection(F&& f) {
		for (SelectionIterator i(this); i < selections.size(); ++i) {
			*i += i.insertion_offset;
			*i -= i.deletion_offset;
			std::forward<F>(f)(i);
		}
	}
	void insert_text(const char* text) {
		for_each_selection([&](SelectionIterator& selection) {
			selection.delete_text();
			selection.insert(text);
		});
	}
	void insert_newline() {
		for_each_selection([&](SelectionIterator& selection) {
			selection.delete_text();
			selection.insert('\n');
			const std::size_t line = buffer.get_info_for_index(selection->head - 1).newlines;
			const std::size_t index = buffer.get_info_for_line_start(line).bytes;
			auto i = buffer.get_iterator(index);
			while (i != buffer.end() && (*i == ' ' || *i == '\t')) {
				selection.insert(*i);
				++i;
			}
		});
	}
	void delete_backward() {
		for_each_selection([&](SelectionIterator& selection) {
			if (selection->is_empty()) {
				selection->head = get_previous_index(selection->head);
			}
			selection.delete_text();
		});
		selections.collapse(true);
	}
	void delete_forward() {
		for_each_selection([&](SelectionIterator& selection) {
			if (selection->is_empty()) {
				selection->head = get_next_index(selection->head);
			}
			selection.delete_text();
		});
		selections.collapse(false);
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
		selections.set_selection(get_index(column, line));
	}
	void toggle_cursor(std::size_t column, std::size_t line) {
		const std::size_t cursor = get_index(column, line);
		const std::size_t index = selections.lower_bound(cursor);
		if (index < selections.size() && selections[index].min() <= cursor) {
			selections.erase(selections.begin() + index);
			if (selections.last_selection == index) {
				selections.last_selection = selections.size() - 1;
			}
			else if (selections.last_selection > index) {
				--selections.last_selection;
			}
		}
		else {
			selections.emplace(selections.begin() + index, cursor);
			selections.last_selection = index;
		}
	}
	void extend_selection(std::size_t column, std::size_t line) {
		Selection& selection = selections.get_last_selection();
		selection.head = get_index(column, line);
		selections.collapse(selection.is_reversed());
	}
	void move_left(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (!extend_selection && !selection.is_empty()) {
				selection = selection.min();
				continue;
			}
			selection.head = get_previous_index(selection.head);
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(true);
	}
	void move_right(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (!extend_selection && !selection.is_empty()) {
				selection = selection.max();
				continue;
			}
			selection.head = get_next_index(selection.head);
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(false);
	}
	void move_up(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (!extend_selection && !selection.is_empty()) {
				selection = selection.min();
				continue;
			}
			selection.head = get_index_above(selection.head);
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(true);
	}
	void move_down(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (!extend_selection && !selection.is_empty()) {
				selection = selection.max();
				continue;
			}
			selection.head = get_index_below(selection.head);
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(false);
	}
	void move_to_beginning_of_word(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (!extend_selection && !selection.is_empty()) {
				selection = selection.min();
				continue;
			}
			std::size_t word_start, word_end;
			get_previous_word(selection.head, word_start, word_end);
			selection.head = word_start;
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(true);
	}
	void move_to_end_of_word(bool extend_selection = false) {
		for (Selection& selection: selections) {
			if (!extend_selection && !selection.is_empty()) {
				selection = selection.max();
				continue;
			}
			std::size_t word_start, word_end;
			get_next_word(selection.head, word_start, word_end);
			selection.head = word_end;
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(false);
	}
	void move_to_beginning_of_line(bool extend_selection = false) {
		for (Selection& selection: selections) {
			const std::size_t line = buffer.get_info_for_index(selection.head).newlines;
			selection.head = buffer.get_info_for_line_start(line).bytes;
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(true);
	}
	void move_to_end_of_line(bool extend_selection = false) {
		for (Selection& selection: selections) {
			const std::size_t line = buffer.get_info_for_index(selection.head).newlines;
			selection.head = buffer.get_info_for_line_end(line).bytes;
			if (!extend_selection) {
				selection.tail = selection.head;
			}
		}
		selections.collapse(false);
	}
	void select_all() {
		selections.set_selection(0, buffer.get_size() - 1);
	}
	const Theme& get_theme() const {
		return prism::get_theme("one-dark");
	}
	std::string copy() const {
		std::string string;
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
		return string;
	}
	std::string cut() {
		std::string result = copy();
		for_each_selection([&](SelectionIterator& selection) {
			selection.delete_text();
		});
		return result;
	}
	void paste(const char* text) {
		std::size_t newlines = 0;
		for (const char* c = text; *c; ++c) {
			newlines += *c == '\n';
		}
		if (newlines + 1 == selections.size()) {
			const char* c = text;
			for_each_selection([&](SelectionIterator& selection) {
				selection.delete_text();
				while (*c != '\n' && *c != '\0') {
					selection.insert(*c);
					++c;
				}
				if (*c == '\n') {
					++c;
				}
			});
		}
		else {
			insert_text(text);
		}
	}
	void save(const char* path) {
		buffer.save(path);
	}
};
