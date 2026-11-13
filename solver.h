#ifndef SOLVER_H
#define SOLVER_H

void search_word(const char *word,
                 const int lines,
                 const int columns,
                 int coords[2][2],
                 const char crossword[lines][columns]);

void check_word(const char *word,
                int coords[2][2],
                const int x,
                const int y,
                const int length,
                const int lines,
                const int columns,
                const char crossword[lines][columns]);

int count_length(const char *word);

void draw_crossword(const char *filepath,
                    const int lines,
                    const int columns,
                    char crossword[lines][columns]);

void count_lines_columns(const char *filepath,
                         int dimensions[2]);

#endif
