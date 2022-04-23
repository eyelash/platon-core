#include "c_api.h"
#include "editor.hpp"

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
	return editor->render(first_line, last_line);
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

void platon_editor_move_to_beginning_of_line(PlatonEditor* editor, int extend_selection) {
	editor->move_to_beginning_of_line(extend_selection);
}

void platon_editor_move_to_end_of_line(PlatonEditor* editor, int extend_selection) {
	editor->move_to_end_of_line(extend_selection);
}

const char* platon_editor_get_theme(const PlatonEditor* editor) {
	return editor->get_theme();
}

const char* platon_editor_copy(const PlatonEditor* editor) {
	return editor->copy();
}

void platon_editor_paste(PlatonEditor* editor, const char* text) {
	editor->paste(text);
}

void platon_editor_save(PlatonEditor* editor, const char* path) {
	editor->save(path);
}
