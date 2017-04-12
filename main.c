#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#include "handlers.h"
#include "parser.h"
#include "line.h"
#include "help.h"
#include "instance.h"

#include <wchar.h>
#include <stdio.h>
#include <string.h>

#define CHECK_WSTREQ1(x, y) if (wcscmp(x, y) == 0)
#define CHECK_WSTREQ2(x1, y1, x2, y2) if (wcscmp(x1, y1) == 0 && wcscmp(x2, y2) == 0)

int main(int argc, char **argv)
{
    instance *help_instance;
    instance *inst;
    wchar_t *word1, *word2;

    setlocale(LC_ALL, "ru_RU.utf8");
    isatty_stdout = (char)isatty(fileno(stdout));
    isatty_stderr = (char)isatty(fileno(stderr));

    initialize_instance(&inst);
    initialize_buffer();

    initialize_help_instance(&help_instance);

    if (argc > 1)
    {
        FILE *file;
        wchar_t *text;
        uint text_capacity, text_position;
        if (argc > 2)   
        {
            show_message(L"unknown command line arguments found", MSG_ERROR);
            return 0;
        }
        file = fopen(argv[1], "r");
        if (!file)
        {
            show_message(L"failed to open file specified in command line argument", MSG_ERROR);
            return 0;
        }
        text_capacity = 4;
        text = (wchar_t*)malloc(sizeof(wchar_t) * text_capacity);
        text[0] = WEOS;
        text_position = 0;
        while (1)
        {
            text[text_position] = fgetwc(file);
            text[text_position + 1] = WEOS;
            if (text[text_position] == WEOF)
            {
                text[text_position] = WEOS;
                break;
            }
            if (!(text_position + 4 < text_capacity))
            {
                text_capacity <<= 1;
                wstring_resize(&text, text_capacity);
            }
            ++text_position;
        }
        fclose(file);
        association = (char*)malloc(sizeof(char) * (strlen(argv[1]) + 3));
        association[0] = '\0';
        strcpy(association, argv[1]);
        {
            line *l, *rubbish;
            uint l_counter;
            l = wstring_to_lines(text, &l_counter, &rubbish);
            inst->begin = l;
            inst->line_counter = l_counter;
        }
        free(text);
    }

    while (1)
    {
        if (isatty_stdout)
            fwprintf(stdout, L"editor: ");
        read_line_to_buffer();
        skip_whitespaces_tabs();

        if (buffer[buffer_position] == WEOS)
            continue;

        parse_word(&word1);

        CHECK_WSTREQ1(word1, L"exit")
        {
            if (exit_handler(inst) == 0)
            {
                free(word1);
                break;
            }
            else
                goto ok1;
        }
        CHECK_WSTREQ1(word1, L"read")
        {
            read_open_handler(&inst, 0);
            goto ok1;
        }
        CHECK_WSTREQ1(word1, L"open")
        {
            read_open_handler(&inst, 1);
            goto ok1;
        }
        CHECK_WSTREQ1(word1, L"write")
        {
            write_handler(inst);
            goto ok1;
        }
        CHECK_WSTREQ1(word1, L"help")
        {
            if (!config_wrap)
                print_range_interactive_nonwrap(help_instance, 1, help_instance->line_counter);
            else
                print_range_interactive_wrap(help_instance, 1, help_instance->line_counter);
            goto ok1;
        }

        skip_whitespaces_tabs();
        if(buffer[buffer_position] == WEOS)
            goto err1;

        parse_word(&word2);
        skip_whitespaces_tabs();
        CHECK_WSTREQ2(word1, L"set", word2, L"tabwidth")
        {
            set_tabwidth_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"set", word2, L"numbers")
        {
            set_numbers_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"print", word2, L"pages")
        {
            if (!inst->line_counter)
            {
                show_message(L"file is empty", MSG_ERROR);
                goto ok2;
            }

            if (isatty_stdout)
            {
                if (!config_wrap)
                    print_range_interactive_nonwrap(inst, 1, inst->line_counter);
                else
                    print_range_interactive_wrap(inst, 1, inst->line_counter);
            }
            else
            {
                uint i;
                line *current = inst->begin;

                for (i = 0; i < inst->line_counter; ++i, current = current->next)
                    fwprintf(stdout, L"%ls\n", current->s);
                fputwc(WNEWLINE, stdout);
            }

            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"print", word2, L"range")
        {
            print_range_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"set", word2, L"wrap")
        {
            set_wrap_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"insert", word2, L"after")
        {
            insert_after_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"edit", word2, L"string")
        {
            edit_string_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"insert", word2, L"symbol")
        {
            insert_symbol_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"replace", word2, L"substring")
        {
            replace_substring_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"delete", word2, L"range")
        {
            delete_range_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"delete", word2, L"braces")
        {
            delete_braces_handler(inst);
            goto ok2;
        }
        CHECK_WSTREQ2(word1, L"set", word2, L"name")
        {
            set_name_handler(inst);
            goto ok2;
        }

        goto err2;

        ok1:
            free(word1);
            continue;
        ok2:
            free(word1);
            free(word2);
            continue;
        err1:
            free(word1);
            show_message(L"unknown command", MSG_ERROR);
            continue;
        err2:
            free(word1);
            free(word2);
            show_message(L"unknown command", MSG_ERROR);
    }

    /* freeeeeeees */
    destroy_buffer();
    destroy_instance(inst);
    destroy_instance(help_instance);
    if (association)
        free(association);

    return 0;
}
