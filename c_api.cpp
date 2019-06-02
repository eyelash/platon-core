#include "c_api.h"
#include "editor.hpp"

struct PlatonEditor: Editor {
	PlatonEditor(const char* path): Editor(path) {}
};

PlatonEditor* platon_editor_new(const char* path) {
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

void platon_editor_insert(PlatonEditor* editor, const char* text) {
	editor->insert(text);
}

void platon_editor_backspace(PlatonEditor* editor) {
	editor->backspace();
}

void platon_editor_set_cursor(PlatonEditor* editor, size_t column, size_t row) {
	editor->set_cursor(column, row);
}

void platon_editor_toggle_cursor(PlatonEditor* editor, size_t column, size_t row) {
	editor->toggle_cursor(column, row);
}

void platon_editor_move_left(PlatonEditor* editor) {
	editor->move_left();
}

void platon_editor_move_right(PlatonEditor* editor) {
	editor->move_right();
}

const char* platon_editor_get_theme(const PlatonEditor* editor) {
	return editor->get_theme();
}
