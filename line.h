#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#ifndef LINE_H
#define LINE_H

#include "parser.h"

#include <stdlib.h>
#include <wchar.h>

struct line_s
{
    wchar_t *s;
    struct line_s *prev, *next;
};
typedef struct line_s line;

void initialize_line(line **l);

void destroy_line(line *l);

uint lines_lengths(line *l);

void set_prev_line(line *l, line *prev);

void set_next_line(line *l, line *next);

void merge_lines(line *l, line *r);

line *advance(line *l, uint n);

void delete_range(line *from, line *to);

void insert_lines(line *after, line *source);

line *wstring_to_lines(const wchar_t *wstring, uint *new_size, line **new_finish);

#endif
