#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlatonEditor PlatonEditor;

PlatonEditor* platon_editor_new(void);
PlatonEditor* platon_editor_new_from_file(const char* path);
void platon_editor_free(PlatonEditor* editor);
size_t platon_editor_get_total_lines(PlatonEditor* editor);
const char* platon_editor_render(PlatonEditor* editor, size_t first_line, size_t last_line);
void platon_editor_insert_text(PlatonEditor* editor, const char* text);
void platon_editor_insert_newline(PlatonEditor* editor);
void platon_editor_delete_backward(PlatonEditor* editor);
void platon_editor_delete_forward(PlatonEditor* editor);
void platon_editor_set_cursor(PlatonEditor* editor, size_t column, size_t row);
void platon_editor_toggle_cursor(PlatonEditor* editor, size_t column, size_t row);
void platon_editor_move_left(PlatonEditor* editor, int extend_selection);
void platon_editor_move_right(PlatonEditor* editor, int extend_selection);
void platon_editor_move_up(PlatonEditor* editor, int extend_selection);
void platon_editor_move_down(PlatonEditor* editor, int extend_selection);
void platon_editor_move_to_beginning_of_line(PlatonEditor* editor, int extend_selection);
void platon_editor_move_to_end_of_line(PlatonEditor* editor, int extend_selection);
const char* platon_editor_get_theme(const PlatonEditor* editor);
const char* platon_editor_copy(const PlatonEditor* editor);
void platon_editor_paste(PlatonEditor* editor, const char* text);
void platon_editor_save(PlatonEditor* editor, const char* path);

#ifdef __cplusplus
}
#endif
