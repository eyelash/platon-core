#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlatonEditor PlatonEditor;

PlatonEditor* platon_editor_new(const char* path);
void platon_editor_free(PlatonEditor* editor);
size_t platon_editor_get_total_lines(PlatonEditor* editor);
const char* platon_editor_render(PlatonEditor* editor, size_t first_line, size_t last_line);

#ifdef __cplusplus
}
#endif
