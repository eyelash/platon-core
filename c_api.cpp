#include "c_api.h"
#include "editor.hpp"
#include "json.hpp"

static void write(JSONWriter& writer, const std::string& s) {
	writer.write_string(s);
}
static void write(JSONWriter& writer, std::size_t n) {
	writer.write_number(n);
}
static void write(JSONWriter& writer, bool b) {
	writer.write_boolean(b);
}
template <class T, std::size_t N> static void write(JSONWriter& writer, const T (&array)[N]) {
	writer.write_array([&](JSONArrayWriter& writer) {
		for (const T& element: array) {
			write(writer.write_element(), element);
		}
	});
}
template <class T> static void write(JSONWriter& writer, const std::vector<T>& array) {
	writer.write_array([&](JSONArrayWriter& writer) {
		for (const T& element: array) {
			write(writer.write_element(), element);
		}
	});
}
static void write(JSONWriter& writer, const Range& range) {
	writer.write_array([&](JSONArrayWriter& writer) {
		writer.write_element().write_number(range.start);
		writer.write_element().write_number(range.end);
	});
}
static void write(JSONWriter& writer, const Span& span) {
	writer.write_array([&](JSONArrayWriter& writer) {
		writer.write_element().write_number(span.start);
		writer.write_element().write_number(span.end);
		writer.write_element().write_number(span.style);
	});
}
static void write(JSONWriter& writer, const RenderedLine& line) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write(writer.write_member("text"), line.text);
		write(writer.write_member("number"), line.number);
		write(writer.write_member("spans"), line.spans);
		write(writer.write_member("selections"), line.selections);
		write(writer.write_member("cursors"), line.cursors);
	});
}
static void write(JSONWriter& writer, const Color& color) {
	writer.write_array([&](JSONArrayWriter& writer) {
		writer.write_element().write_number(color.r * 255.f + .5f);
		writer.write_element().write_number(color.g * 255.f + .5f);
		writer.write_element().write_number(color.b * 255.f + .5f);
		writer.write_element().write_number(color.a * 255.f + .5f);
	});
}
static void write(JSONWriter& writer, const Style& style) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write(writer.write_member("color"), style.color);
		write(writer.write_member("bold"), style.bold);
		write(writer.write_member("italic"), style.italic);
	});
}
static void write(JSONWriter& writer, const Theme& theme) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write(writer.write_member("background"), theme.background);
		write(writer.write_member("background_active"), theme.background_active);
		write(writer.write_member("selection"), theme.selection);
		write(writer.write_member("cursor"), theme.cursor);
		write(writer.write_member("gutter_background"), theme.gutter_background);
		write(writer.write_member("gutter_background_active"), theme.gutter_background_active);
		write(writer.write_member("styles"), theme.styles);
	});
}

struct PlatonEditor: Editor {
	PlatonEditor() {}
	PlatonEditor(const char* path): Editor(path) {}
};

PlatonEditor* platon_editor_new() {
	return new PlatonEditor();
}

PlatonEditor* platon_editor_new_from_file(const char* path) {
	return new PlatonEditor(path);
}

void platon_editor_free(PlatonEditor* editor) {
	delete editor;
}

size_t platon_editor_get_total_lines(PlatonEditor* editor) {
	return editor->get_total_lines();
}

const char* platon_editor_render(PlatonEditor* editor, size_t first_line, size_t last_line) {
	static std::string json;
	json.clear();
	JSONWriter writer(json);
	write(writer, editor->render(first_line, last_line));
	return json.c_str();
}

void platon_editor_insert_text(PlatonEditor* editor, const char* text) {
	editor->insert_text(text);
}

void platon_editor_insert_newline(PlatonEditor* editor) {
	editor->insert_newline();
}

void platon_editor_delete_backward(PlatonEditor* editor) {
	editor->delete_backward();
}

void platon_editor_delete_forward(PlatonEditor* editor) {
	editor->delete_forward();
}

void platon_editor_set_cursor(PlatonEditor* editor, size_t column, size_t line) {
	editor->set_cursor(column, line);
}

void platon_editor_toggle_cursor(PlatonEditor* editor, size_t column, size_t line) {
	editor->toggle_cursor(column, line);
}

void platon_editor_extend_selection(PlatonEditor* editor, size_t column, size_t line) {
	editor->extend_selection(column, line);
}

void platon_editor_move_left(PlatonEditor* editor, int extend_selection) {
	editor->move_left(extend_selection);
}

void platon_editor_move_right(PlatonEditor* editor, int extend_selection) {
	editor->move_right(extend_selection);
}

void platon_editor_move_up(PlatonEditor* editor, int extend_selection) {
	editor->move_up(extend_selection);
}

void platon_editor_move_down(PlatonEditor* editor, int extend_selection) {
	editor->move_down(extend_selection);
}

void platon_editor_move_to_beginning_of_word(PlatonEditor* editor, int extend_selection) {
	editor->move_to_beginning_of_word(extend_selection);
}

void platon_editor_move_to_end_of_word(PlatonEditor* editor, int extend_selection) {
	editor->move_to_end_of_word(extend_selection);
}

void platon_editor_move_to_beginning_of_line(PlatonEditor* editor, int extend_selection) {
	editor->move_to_beginning_of_line(extend_selection);
}

void platon_editor_move_to_end_of_line(PlatonEditor* editor, int extend_selection) {
	editor->move_to_end_of_line(extend_selection);
}

void platon_editor_select_all(PlatonEditor* editor) {
	editor->select_all();
}

const char* platon_editor_get_theme(const PlatonEditor* editor) {
	static std::string json;
	json.clear();
	JSONWriter writer(json);
	write(writer, editor->get_theme());
	return json.c_str();
}

const char* platon_editor_copy(const PlatonEditor* editor) {
	static std::string string;
	string = editor->copy();
	return string.c_str();
}

const char* platon_editor_cut(PlatonEditor* editor) {
	static std::string string;
	string = editor->cut();
	return string.c_str();
}

void platon_editor_paste(PlatonEditor* editor, const char* text) {
	editor->paste(text);
}

void platon_editor_save(PlatonEditor* editor, const char* path) {
	editor->save(path);
}
