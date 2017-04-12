#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#ifndef HANDLERS_H
#define HANDLERS_H

#include "instance.h"
#include "parser.h"
#include "help.h"

#include <stdio.h>
#include <wchar.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern char isatty_stdout;
extern char isatty_stderr;

extern char changes_made;
extern char *association;

extern char config_use_cls;

#define ANSI_CLEAR_SCREEN  L"\033[2J"

#define ANSI_COLOR_RED     L"\x1b[31m"
#define ANSI_COLOR_GREEN   L"\x1b[32m"
#define ANSI_COLOR_YELLOW  L"\x1b[33m"
#define ANSI_COLOR_BLUE    L"\x1b[34m"
#define ANSI_COLOR_MAGENTA L"\x1b[35m"
#define ANSI_COLOR_CYAN    L"\x1b[36m"
#define ANSI_COLOR_RESET   L"\x1b[0m"

#define INVITE_TAG L"editor:"
#define ERROR_TAG L"error:"
#define SUCCESS_TAG L"success:"

#define CHECK_WEOS_MISSING \
    if (buffer[buffer_position] == WEOS)\
    {\
        show_message(L"missing arguments", MSG_ERROR);\
        return;\
    }

#define CHECK_WEOS_UNEXPECTED \
    if (buffer[buffer_position] != WEOS)\
    {\
        show_message(L"unexpected arguments found", MSG_ERROR);\
        return;\
    }

#define CHECK_WEOS_ESCAPE \
    if (buffer[buffer_position] == WEOS)\
    {\
        show_message(L"escape character \\ must have succeeding character", MSG_ERROR);\
        return;\
    }


#define CHECK_PARSE_UINT(x) \
    if (parse_uint(x) == -1)\
    {\
        show_message(L"unsigned integers expected as argument", MSG_ERROR);\
        return;\
    }

void swap_uint(uint *x, uint *y);

uint uint_len(uint x);

#define MSG_NOMODE 0
#define MSG_ERROR 1
#define MSG_SUCCESS 2
void show_message(const wchar_t *msg, int mode);

/*
 * for interactive mode
 */
struct termios old_termios;
void set_noncanon_nonecho();
void unset_noncanon_nonecho();

#define KEY_LEFTARROW 1
#define KEY_RIGHTARROW 2
#define KEY_UPARROW 3
#define KEY_DOWNARROW 4
#define KEY_SPACE 5
#define KEY_Q 6
#define KEY_OTHER 7
int read_interactive_keypress();

#define BE_SUCCESS 0
#define BE_WRONGBALANCE 1
int bracets_eraser(line *from, uint range, line **res, uint *res_size, line **res_end);

int print_line_interactive(wchar_t *wstring, uint start_position, uint symbols_limit,
                           uint offset,
                           char wrapped,
                           uint number);

/*
 * non-wrap version
 * with < > ^ v SPACE
 */
void print_range_interactive_nonwrap(instance *inst, const uint from, const uint to);

void print_range_interactive_wrap(instance *inst, const uint from, const uint to);

/*
 * Commands for text viewing
 */
void set_tabwidth_handler(instance *inst);

void set_numbers_handler(instance *inst);

void print_range_handler(instance *inst);

void set_wrap_handler(instance *inst);


/*
 * Commands for pasting strings
 */
void insert_after_handler(instance *inst);


/*
 * Commands for editing strings
 */
void edit_string_handler(instance *inst);

void insert_symbol_handler(instance *inst);

void replace_substring_handler(instance *inst);

void delete_range_handler(instance *inst);

void delete_braces_handler(instance *inst);

/*
 * etc
 */
int exit_handler(instance *inst);

void read_open_handler(instance **inst, const char remember_association);

void write_handler(instance *inst);

void set_name_handler(instance *inst);

#endif
