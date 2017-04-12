#include "line.h"

void initialize_line(line **l)
{
    *l = (line*)malloc(sizeof(line));
    (*l)->prev = (*l)->next = NULL;
}

void destroy_line(line *l)
{
    if (l->s)
        free(l->s);
    free(l);
}

uint lines_lengths(line *l)
{
    uint res = 0;
    while (l)
    {
        ++res;
        l = l->next;
    }
    return res;
}

void set_prev_line(line *l, line *prev)
{
    if (l == NULL)
        return;
    l->prev = prev;
}

void set_next_line(line *l, line *next)
{
    if (l == NULL)
        return;
    l->next = next;
}

void merge_lines(line *l, line *r)
{
    set_next_line(l, r);
    set_prev_line(r, l);
}

line *advance(line *l, uint n)
{
    uint i = 0;
    for (i = 0; l->next && i < n; ++i)
        l = l->next;
    return l;
}

void delete_range(line *from, line *to)
{
    line *temp;

    merge_lines(from->prev, to->next);

    while (from != to)
    {
        temp = from->next;
        destroy_line(from);
        from = temp;
    }
    destroy_line(to);
}

void insert_lines(line *after, line *source)
{
    line *end_of_source, *temp_next;
    if (!source || !after)
        return;

    end_of_source = source;
    temp_next = after->next;

    while (end_of_source->next)
        end_of_source = end_of_source->next;

    merge_lines(after, source);
    merge_lines(end_of_source, temp_next);
}

line *wstring_to_lines(const wchar_t *wstring, uint *new_size, line **new_finish)
{
    line *begin;
    line *current;
    uint current_capacity;
    uint current_position;
    uint wstring_position = 0;

    initialize_line(&begin);
    current_capacity = 2;
    current_position = 0;
    begin->s = (wchar_t*)malloc(sizeof(wchar_t) * current_capacity);
    begin->s[0] = WEOS;
    *new_finish = begin;

    current = begin;
    *new_size = 1;
    while (wstring[wstring_position] != WEOS)
    {
        if (wstring[wstring_position] != WNEWLINE)
        {
            if (!(current_position + 1 < current_capacity))
            {
                current_capacity <<= 1;
                wstring_resize(&current->s, current_capacity);
            }
            current->s[current_position] = wstring[wstring_position];
            current->s[++current_position] = WEOS;
        }
        else
        {
            line *new_line;

            initialize_line(&new_line);
            *new_finish = new_line;
            current_capacity = 2;
            current_position = 0;
            new_line->s = (wchar_t*)malloc(sizeof(wchar_t) * current_capacity);
            new_line->s[0] = WEOS;

            merge_lines(current, new_line);
            current = new_line;
            ++*new_size;
        }
        ++wstring_position;
    }

    return begin;
}
