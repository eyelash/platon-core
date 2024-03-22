#include "c_api.h"
#include "editor.hpp"
#include "json.hpp"

static void write_color(const Color& color, JSONWriter& writer) {
	writer.write_array([&](JSONArrayWriter& writer) {
		writer.write_element().write_number(color.r * 255.f + .5f);
		writer.write_element().write_number(color.g * 255.f + .5f);
		writer.write_element().write_number(color.b * 255.f + .5f);
		writer.write_element().write_number(color.a * 255.f + .5f);
	});
}
static void write_style(const Style& style, JSONWriter& writer) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write_color(style.color, writer.write_member("color"));
		writer.write_member("bold").write_boolean(style.bold);
		writer.write_member("italic").write_boolean(style.italic);
	});
}
static void write_theme(const Theme& theme, JSONWriter& writer) {
	writer.write_object([&](JSONObjectWriter& writer) {
		write_color(theme.background, writer.write_member("background"));
		write_color(theme.background_active, writer.write_member("background_active"));
		write_color(theme.selection, writer.write_member("selection"));
		write_color(theme.cursor, writer.write_member("cursor"));
		write_color(theme.number_background, writer.write_member("number_background"));
		write_color(theme.number_background_active, writer.write_member("number_background_active"));
		write_style(theme.number, writer.write_member("number"));
		write_style(theme.number_active, writer.write_member("number_active"));
		writer.write_member("styles").write_array([&](JSONArrayWriter& writer) {
			for (const Style& style: theme.styles) {
				write_style(style, writer.write_element());
			}
		});
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
	writer.write_array([&](JSONArrayWriter& writer) {
		for (const RenderedLine& line: editor->render(first_line, last_line)) {
			writer.write_element().write_object([&](JSONObjectWriter& writer) {
				writer.write_member("text").write_string(line.text);
				writer.write_member("number").write_number(line.number);
				writer.write_member("spans").write_array([&](JSONArrayWriter& writer) {
					for (const Span& span: line.spans) {
						writer.write_element().write_array([&](JSONArrayWriter& writer) {
							writer.write_element().write_number(span.start);
							writer.write_element().write_number(span.end);
							writer.write_element().write_number(span.style - Style::DEFAULT);
						});
					}
				});
				writer.write_member("selections").write_array([&](JSONArrayWriter& writer) {
					for (const Selection& selection: line.selections) {
						writer.write_element().write_array([&](JSONArrayWriter& writer) {
							writer.write_element().write_number(selection.first);
							writer.write_element().write_number(selection.last);
						});
					}
				});
				writer.write_member("cursors").write_array([&](JSONArrayWriter& writer) {
					for (std::size_t cursor: line.cursors) {
						writer.write_element().write_number(cursor);
					}
				});
			});
		}
	});
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
	write_theme(editor->get_theme(), writer);
	return json.c_str();
}

const char* platon_editor_copy(const PlatonEditor* editor) {
	return editor->copy();
}

const char* platon_editor_cut(PlatonEditor* editor) {
	return editor->cut();
}

void platon_editor_paste(PlatonEditor* editor, const char* text) {
	editor->paste(text);
}

void platon_editor_save(PlatonEditor* editor, const char* path) {
	editor->save(path);
}
