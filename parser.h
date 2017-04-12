#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <assert.h>

#define WEOS L'\0'
#define WNEWLINE L'\n'
#define WTAB L'\t'
#define WSPACE L' '

#define INITIAL_BUFFER_CAPACITY 4096

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

extern const wchar_t escape_characters[];
extern const wchar_t escape_characters_replaced[];

typedef unsigned int uint;

int escape_character_type(wchar_t c1, wchar_t c2);

void wstring_resize(wchar_t **wstring, uint new_size);

/*
 * Don't forget to free result before use
 */
void wstring_substr(wchar_t *wstring, wchar_t **result, uint position, uint size);

/*
 * Don't forget to free left and right before use
 */
void wstring_split(wchar_t *wstring, wchar_t **left, wchar_t **right, uint left_size);

/*
 * Don't forget to free wstring
 */
void wstring_merge(wchar_t **wstring, wchar_t *left, wchar_t *right);

wchar_t *buffer;
uint buffer_capacity;
uint buffer_position;

void initialize_buffer();

void destroy_buffer();

int read_line_to_buffer();

void skip_whitespaces_tabs();

int parse_wchar(wchar_t *result);

/*
 * Don't forger to free result before use
 */
int parse_word(wchar_t **result);

int parse_uint(uint *result);

#define PSQ_SUCCESS 0
#define PSQ_NO_CLOSING_QUOTE 1
#define PSQ_UNKNOWN_ESCAPE_SEQ 2
/*
 * Don't forget to free result before use
 *
 * If error happens, result is freed automatically in this function :|
 */
int parse_single_quotes(wchar_t **result);

/*
 * don't forget to free result before use
 *
 * if error happens, result is freed automatically in this function
 */
#define PF_SUCCESS 0
#define PF_NO_CLOSING_QUOTE 1
#define PF_UNKNOWN_SEQUENCE 2
int parse_filename(char **result);

/*
 * finds all occurrences of pattern in text
 * don't forget to free result before use
 */
void wstrstr(wchar_t *text, wchar_t *pattern, uint **result, uint *counter);

#endif
