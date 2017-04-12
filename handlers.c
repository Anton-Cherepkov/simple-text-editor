#include "handlers.h"

char isatty_stdout;
char isatty_stderr;

char changes_made = 0;
char *association = NULL;

char config_use_cls = 0;

void swap_uint(uint *x, uint *y)
{
    *x ^= *y;
    *y ^= *x;
    *x ^= *y;
}

uint uint_len(uint x)
{
    uint len = 0;
    if (x == 0)
        return 1;
    while (x > 0)
    {
        ++len;
        x /= 10;
    }
    return len;
}

void show_message(const wchar_t *msg, int mode)
{
    if (mode == MSG_ERROR) {
        if (!isatty_stderr)
            fwprintf(stderr, L"%ls ", ERROR_TAG);
        else
            fwprintf(stderr, L"%ls%ls%ls ", ANSI_COLOR_RED, ERROR_TAG, ANSI_COLOR_RESET);
        fwprintf(stderr, L"%ls\n", msg);
    }
}

void set_noncanon_nonecho()
{
    struct termios new_termios;

    tcgetattr(0, &old_termios);

    memcpy(&new_termios, &old_termios, sizeof(struct termios));
    new_termios.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(0, TCSANOW, &new_termios);
}

void unset_noncanon_nonecho()
{
    tcsetattr(0, TCSANOW, &old_termios);
}

int read_interactive_keypress()
{
    switch (fgetwc(stdin))
    {
        case L' ':
            return KEY_SPACE;
        case L'q':
            return KEY_Q;
        case L'\033':
            fgetwc(stdin);
            switch (fgetwc(stdin))
            {
                case L'A':
                    return KEY_UPARROW;
                case L'B':
                    return KEY_DOWNARROW;
                case L'C':
                    return KEY_RIGHTARROW;
                case L'D':
                    return KEY_LEFTARROW;
                default:
                    return KEY_OTHER;
            }
        default:
            return KEY_OTHER;
    }
}

int bracets_eraser(line *from, uint range, line **res, uint *res_size, line **res_end)
{
    line *current;
    uint i, j;
    int balance = 0;

    wchar_t *repl;
    uint repl_capacity = 8;
    uint repl_position = 0;

    repl = (wchar_t*)malloc(sizeof(wchar_t) * repl_capacity);
    repl[0] = WEOS;
    for (i = 0, current = from; i < range; ++i, current = current->next)
    {
        for (j = 0; current->s[j] != WEOS; ++j)
        {
            if (!(repl_position + 4 < repl_capacity))
            {
                repl_capacity <<= 1;
                wstring_resize(&repl, repl_capacity);
            }

            if (current->s[j] == L'{')
                ++balance;
            else if (current->s[j] == L'}')
                --balance;
            else if (balance == 0)
            {
                repl[repl_position] = current->s[j];
                repl[++repl_position] = WEOS;
            }

            if (balance < 0)
            {
                free(repl);
                return BE_WRONGBALANCE;
            }
        }

        if (i != range - 1 && balance == 0)
        {
            repl[repl_position] = WNEWLINE;
            repl[++repl_position] = WEOS;
        }
    }

    *res = wstring_to_lines(repl, res_size, res_end);
    free(repl);
    return BE_SUCCESS;
}

int print_line_interactive(wchar_t *wstring, uint start_position, uint symbols_limit,
                           uint offset,
                           char wrapped,
                           uint number)
{
    uint cnt = 0;
    uint wstring_position = start_position;
    uint symbols_printed = 0;
    uint i;

    fwprintf(stdout, L"%ls", ANSI_COLOR_CYAN);
    if (config_wrap && wrapped)
        fwprintf(stdout, L"%*ls", offset, L" ");
    else
    {
        if (config_numbers)
            fwprintf(stdout, L"%-*u", offset, number);
        else
            fwprintf(stdout, L"%-*ls", offset, L"|");

    }
    fwprintf(stdout, L"%ls", ANSI_COLOR_RESET);
    symbols_printed += offset;

    while (symbols_printed < symbols_limit && wstring[wstring_position] != WEOS)
    {
        if (wstring[wstring_position] == L'\t')
        {
            for (i = 0; i < config_tabwidth && symbols_printed < symbols_limit; ++i, ++symbols_printed)
                fputwc(L' ', stdout);
        }
        else
        {
            fputwc(wstring[wstring_position], stdout);
            ++symbols_printed;
        }
        ++cnt;
        ++wstring_position;
    }

    return cnt;
}

void print_range_interactive_nonwrap(instance *inst, const uint from, const uint to)
{
    uint ws_row, ws_col;

    uint i;
    line *current;

    line *page_from_line;
    uint page_from, page_to;
    uint page_shift = 0;
    uint page_offset;
    uint *line_len;

    {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        if (ws.ws_row <= 1)
            return;
        ws_row = (uint)ws.ws_row - 1;
        ws_col = (uint)ws.ws_col;
    }

    {
        page_from_line = advance(inst->begin, from - 1);
        page_from = from;
        page_to = MIN(from + ws_row - 1, to);

        line_len = (uint*)malloc(sizeof(uint) * (ws_row));
        for (i = 0, current = page_from_line; i < 1 + page_to - page_from; ++i, current = current->next)
            line_len[i] = (uint)wcslen(current->s);
    }

    set_noncanon_nonecho();
    while (1)
    {
        int key_pressed;

        uint right_more = 0;
        page_offset = uint_len(page_to) + 1;

        for (i = 0, current = page_from_line; i < 1 + page_to - page_from; ++i, current = current->next)
        {
            /* bugs here lol */
            /* if (page_shift + ws_col - page_offset < line_len[i])
                right_more |= 1; */
            right_more = 1;
            print_line_interactive(current->s, MIN(page_shift, line_len[i]), ws_col, page_offset, 0, page_from + i);
            putwc(WNEWLINE, stdout);
        }
        if (1 + page_to - page_from < ws_row)
            for (i = 0; i < ws_row - (1 + page_to - page_from); ++i)
                putwc(WNEWLINE, stdout);

        while (1)
        {
            key_pressed = read_interactive_keypress();
            if (key_pressed == KEY_RIGHTARROW)
            {
                if (!right_more)
                    continue;
                ++page_shift;
                break;
            }
            else if (key_pressed == KEY_LEFTARROW)
            {
                if (!page_shift)
                    continue;
                --page_shift;
                break;
            }
            else if (key_pressed == KEY_DOWNARROW)
            {
                if (page_to == to)
                    continue;

                page_from_line = page_from_line->next;
                ++page_to;
                ++page_from;

                for (i = 0, current = page_from_line; i < page_to - page_from; ++i, current = current->next)
                    line_len[i] = line_len[i + 1];
                line_len[page_to - page_from] = (uint)wcslen(current->s);

                break;
            }
            else if (key_pressed == KEY_UPARROW)
            {
                if (page_from == from)
                    continue;

                if (1 + page_to - page_from < ws_row)
                {
                    uint old_page_from = page_from;

                    page_to = page_from - 1;
                    if (page_to >= ws_row)
                        page_from = page_to - ws_row + 1;
                    else
                        page_from = 1;

                    for (i = 0; i < old_page_from - page_from; ++i)
                        page_from_line = page_from_line->prev;

                    for (i = 0, current = page_from_line; i < 1 + page_to - page_from; ++i, current = current->next)
                        line_len[i] = (uint)wcslen(current->s);
                    break;
                }
                else
                {
                    page_from_line = page_from_line->prev;
                    --page_from;
                    --page_to;

                    for (i = page_to - page_from; i > 0; --i)
                        line_len[i] = line_len[i - 1];
                    line_len[0] = (uint) wcslen(page_from_line->s);
                }

                break;
            }
            else if (key_pressed == KEY_SPACE)
            {
                if (page_to == to)
                    continue;

                page_from_line = advance(page_from_line, 1 + page_to - page_from);
                page_from = page_to + 1;
                page_to = MIN(page_from + ws_row - 1, to);

                for (i = 0, current = page_from_line; i < 1 + page_to - page_from; ++i, current = current->next)
                    line_len[i] = (uint)wcslen(current->s);

                break;
            }
            else if (key_pressed == KEY_Q)
                goto br;
        }
        if (config_use_cls)
            fwprintf(stdout, L"%ls", ANSI_CLEAR_SCREEN);

    }
    br:
    unset_noncanon_nonecho();

    free(line_len);
}

void print_range_interactive_wrap(instance *inst, const uint from, const uint to)
{
    uint ws_row, ws_col;

    line *current;
    uint current_number;
    uint current_s_position;
    uint rows_used;

    uint i;

    {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        if (ws.ws_row <= 1)
            return;
        ws_row = (uint)ws.ws_row - 1;
        ws_col = (uint)ws.ws_col;
    }

    current = advance(inst->begin, from - 1);
    current_number = from;
    current_s_position = 0;
    set_noncanon_nonecho();
    while (1)
    {
        int key_pressed;

        rows_used = 0;

        while (rows_used < ws_row && current_number != to + 1)
        {
            current_s_position += print_line_interactive(current->s, current_s_position, ws_col, uint_len(to) + 1, current_s_position != 0, current_number);
            ++rows_used;
            if (current->s[current_s_position] == WEOS)
            {
                current = current->next;
                current_s_position = 0;
                ++current_number;
            }
            fputwc(L'\n', stdout);
        }

        if (rows_used < ws_row)
            for (i = 0; i < ws_row - rows_used; ++i)
                fputwc(L'\n', stdout);

        while (1)
        {
            key_pressed = read_interactive_keypress();
            if (key_pressed == KEY_SPACE && current_number != to + 1)
                break;
            else if (key_pressed == KEY_Q)
                goto br;
        }
    }
    br: unset_noncanon_nonecho();
    return;
}

void set_tabwidth_handler(instance *inst)
{
    uint new_tabwidth;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    CHECK_PARSE_UINT(&new_tabwidth)
    skip_whitespaces_tabs();
    CHECK_WEOS_UNEXPECTED

    config_tabwidth = new_tabwidth;
}

void set_numbers_handler(instance *inst)
{
    wchar_t *word;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    parse_word(&word);
    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        free(word);
        show_message(L"unexpected arguments found", MSG_ERROR);
        return;
    }
    if (wcscmp(word, L"yes") == 0)
        config_numbers = 1;
    else if (wcscmp(word, L"no") == 0)
        config_numbers = 0;
    else
        show_message(L"expected \'yes\' or \'no\'", MSG_ERROR);
    free(word);
}

void print_range_handler(instance *inst)
{
    uint from, to;

    if (!inst->line_counter)
    {
        show_message(L"file is empty", MSG_ERROR);
        return;
    }

    skip_whitespaces_tabs();
    if (buffer[buffer_position] == WEOS)
        from = 1, to = inst->line_counter;
    else
    {
        CHECK_PARSE_UINT(&from)
        skip_whitespaces_tabs();
        if (buffer[buffer_position] == WEOS)
            to = inst->line_counter;
        else
        {
            CHECK_PARSE_UINT(&to)
            skip_whitespaces_tabs();
            CHECK_WEOS_UNEXPECTED
        }
    }

    if (from == 0 || to == 0 || from > inst->line_counter || to > inst->line_counter)
    {
        show_message(L"index out of range", MSG_ERROR);
        return;
    }

    if (from > to)
        swap_uint(&from, &to);

    if (isatty_stdout)
    {
        if (!config_wrap)
            print_range_interactive_nonwrap(inst, from, to);
        else
            print_range_interactive_wrap(inst, from, to);
    }
    else
    {
        uint i;
        line *current = advance(inst->begin, from - 1);

        for (i = 0; i < 1 + to - from; ++i, current = current->next)
            fwprintf(stdout, L"%ls\n", current->s);
        fputwc(WNEWLINE, stdout);
    }
}

void set_wrap_handler(instance *inst)
{
    wchar_t *word;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    parse_word(&word);
    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        free(word);
        show_message(L"unexpected arguments found", MSG_ERROR);
        return;
    }
    if (wcscmp(word, L"yes") == 0)
        config_wrap = 1;
    else if (wcscmp(word, L"no") == 0)
        config_wrap = 0;
    else
        show_message(L"expected \'yes\' or \'no\'", MSG_ERROR);
    free(word);
}

void insert_after_handler(instance *inst)
{
    uint N;
    wchar_t *accumulator;
    line *new_lines;
    line *new_lines_finish;
    uint new_lines_length;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    if (buffer[buffer_position] >= L'0' && buffer[buffer_position] <= L'9') {
        CHECK_PARSE_UINT(&N)
        skip_whitespaces_tabs();
        if (N > inst->line_counter)
            N = inst->line_counter;
    } else {
        N = inst->line_counter;
    }

    if (buffer[buffer_position] != L'\"')
    {
        show_message(L"opening quoutes are not found", MSG_ERROR);
        return;
    }

    if (buffer[buffer_position + 1] != WEOS && buffer[buffer_position + 2] != WEOS && buffer[buffer_position + 1] == L'\"' && buffer[buffer_position + 2] == L'\"')
    {
        uint accumulator_capacity;
        uint accumulator_position;

        buffer_position += 3;
        skip_whitespaces_tabs();
        if (buffer[buffer_position] != WEOS)
        {
            show_message(L"text to be inserted must start with a new line", MSG_ERROR);
            return;
        }

        accumulator_capacity = 8;
        accumulator = (wchar_t*)malloc(sizeof(wchar_t) * accumulator_capacity);
        accumulator[0] = WEOS;
        accumulator_position = 0;

        while (1)
        {
            read_line_to_buffer();
            while (1)
            {
                if (!(accumulator_position + 3 < accumulator_capacity))
                {
                    accumulator_capacity <<= 1;
                    wstring_resize(&accumulator, accumulator_capacity);
                }

                if (buffer[buffer_position] == WEOS)
                    break;

                if (buffer[buffer_position] == L'\"')
                {
                    if (buffer[buffer_position + 1] == L'\"' && buffer[buffer_position + 2] == L'\"')
                    {
                        buffer_position += 3;
                        skip_whitespaces_tabs();
                        if (buffer[buffer_position] == WEOS)
                        {
                            goto parsing_finished;
                        }
                        else
                        {
                            show_message(L"unexpected characters found after closing quotes", MSG_ERROR);
                            free(accumulator);
                            return;
                        }
                    }
                    else
                    {
                        show_message(L"text to be inserted must be finished with three quotes", MSG_ERROR);
                        free(accumulator);
                        return;
                    }
                }

                if (buffer[buffer_position] == L'\\')
                {
                    if (buffer[buffer_position + 1] == L'\"')
                    {
                        accumulator[accumulator_position] = L'\"';
                        accumulator[accumulator_position + 1] = WEOS;
                    }
                    else
                    {
                        int escape_char_type = escape_character_type(buffer[buffer_position], buffer[buffer_position + 1]);
                        if (escape_char_type == -1)
                        {
                            show_message(L"unknown escape sequence", MSG_ERROR);
                            free(accumulator);
                            return;
                        }
                        else
                        {
                            accumulator[accumulator_position] = escape_characters_replaced[escape_char_type];
                            accumulator[accumulator_position + 1] = WEOS;
                        }
                    }
                    ++buffer_position; /* ok, the next char is processed already, skip it */
                }
                else
                {
                    accumulator[accumulator_position] = buffer[buffer_position];
                    accumulator[accumulator_position + 1] = WEOS;
                }

                ++buffer_position, ++accumulator_position;
            }

            accumulator[accumulator_position] = L'\n';
            accumulator[++accumulator_position] = WEOS;
        }
    }
    else
    {
        int parse_result = parse_single_quotes(&accumulator);
        switch (parse_result)
        {
            case PSQ_SUCCESS:
                break;
            case PSQ_NO_CLOSING_QUOTE:
                show_message(L"closing quotes are not found", MSG_ERROR);
                return;
            case PSQ_UNKNOWN_ESCAPE_SEQ:
                show_message(L"unknown escape sequence", MSG_ERROR);
                return;
            default:
                show_message(L"unknown error", MSG_ERROR);
                return;
        }
        skip_whitespaces_tabs();
        if (buffer[buffer_position] != WEOS)
        {
            show_message(L"unexpected characters found after closing quotes", MSG_ERROR);
            free(accumulator);
            return;
        }
    }

    parsing_finished:
    new_lines = wstring_to_lines(accumulator, &new_lines_length, &new_lines_finish);
    free(accumulator);

    if (N == 0)
    {
        insert_lines(new_lines_finish, inst->begin);
        inst->begin = new_lines;
    }
    else
    {
        line *after = advance(inst->begin, N - 1);
        insert_lines(after, new_lines);
    }
    inst->line_counter += new_lines_length;

    changes_made = 1;
}

void edit_string_handler(instance *inst)
{
    uint N, M;
    line *l;
    wchar_t ch1, ch2;
    int escape_type;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    CHECK_PARSE_UINT(&N)
    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    CHECK_PARSE_UINT(&M)
    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    parse_wchar(&ch1);
    if (ch1 == L'\\' && buffer[buffer_position] == L'#')
        ch1 = L'#';
    else if (ch1 == L'\\' && buffer[buffer_position] == L' ')
        ch1 = L' ';
    else if (ch1 == L'\\')
    {
        CHECK_WEOS_ESCAPE
        parse_wchar(&ch2);
        escape_type = escape_character_type(ch1, ch2);
        if (escape_type == -1)
        {
            show_message(L"unknown escape sequence", MSG_ERROR);
            return;
        }
        ch1 = escape_characters_replaced[escape_type];
    }
    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        show_message(L"one symbol is expected only", MSG_ERROR);
        return;
    }

    if (N == 0 || N > inst->line_counter)
    {
        show_message(L"index out of bounds", MSG_ERROR);
        return;
    }
    l = advance(inst->begin, N - 1);
    if (M == 0 || M > wcslen(l->s))
    {
        show_message(L"index out of bounds", MSG_ERROR);
        return;
    }

    if (ch1 != WNEWLINE)
        (l->s)[M - 1] = ch1;
    else
    {
        line *new_line, *temp_next;
        wchar_t *s1, *s2, *s21, *s22;

        wstring_split(l->s, &s1, &s2, M - 1);
        wstring_split(s2, &s21, &s22, 1);
        free(s21);

        temp_next = l->next;
        initialize_line(&new_line);
        l->s = s1;
        new_line->s = s22;
        merge_lines(l, new_line);
        merge_lines(new_line, temp_next);
        inst->line_counter++;
    }

    changes_made = 1;
}

void insert_symbol_handler(instance *inst)
{
    uint N, M;
    line *l;
    uint l_len;
    wchar_t ch1, ch2;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    CHECK_PARSE_UINT(&N)
    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    CHECK_PARSE_UINT(&M)
    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    parse_wchar(&ch1);

    if (ch1 == L'\\' && buffer[buffer_position] == L'#')
        ch1 = L'#';
    else if (ch1 == L'\\' && buffer[buffer_position] == L' ')
        ch1 = L' ';
    else if (ch1 == L'\\')
    {
        int escape_type;
        CHECK_WEOS_ESCAPE
        parse_wchar(&ch2);
        escape_type = escape_character_type(ch1, ch2);
        if (escape_type == -1)
        {
            show_message(L"unknown escape sequence", MSG_ERROR);
            return;
        }
        ch1 = escape_characters_replaced[escape_type];
    }
    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        show_message(L"one symbol is expected only", MSG_ERROR);
        return;
    }

    if (N == 0 || N > inst->line_counter)
    {
        show_message(L"index out of bounds", MSG_ERROR);
        return;
    }
    l = advance(inst->begin, N - 1);
    l_len = (uint)wcslen(l->s);
    if (M == 0)
        M = 1;
    else if (M > l_len + 1)
        M = l_len + 1;

    if (ch1 != WNEWLINE)
    {
        wchar_t *left, *right;
        wstring_split(l->s, &left, &right, M - 1);
        wstring_resize(&left, M + 3);
        left[M - 1] = ch1;
        left[M] = WEOS;
        wstring_merge(&l->s, left, right);
    }
    else
    {
        line *new_line, *temp_next;
        wchar_t *s1, *s2;

        wstring_split(l->s, &s1, &s2, M - 1);

        temp_next = l->next;
        initialize_line(&new_line);
        l->s = s1;
        new_line->s = s2;
        merge_lines(l, new_line);
        merge_lines(new_line, temp_next);
        inst->line_counter++;
    }

    changes_made = 1;
}

void replace_substring_handler(instance *inst)
{
    uint from, to;
    line *from_line, *to_line;
    char mode;

    wchar_t *pattern;
    uint pattern_len;
    uint pattern_capacity, pattern_position;

    wchar_t *replacement;
    uint replacement_len;
    int parse_replacement_res;

    wchar_t *accumulator = NULL;

    line *new_lines;
    line *new_lines_finish;
    uint new_lines_length;

    if (!inst->line_counter)
    {
        show_message(L"index out of bounds", MSG_ERROR);
        return;
    }

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    if (buffer[buffer_position] >= L'0' && buffer[buffer_position] <= L'9')
    {
        CHECK_PARSE_UINT(&from);
        if (from == 0 || from > inst->line_counter)
        {
            show_message(L"index out of bounds", MSG_ERROR);
            return;
        }
        skip_whitespaces_tabs();
        CHECK_WEOS_MISSING
        if (buffer[buffer_position] >= L'0' && buffer[buffer_position] <= L'9')
        {
            CHECK_PARSE_UINT(&to)
            if (to == 0 || to > inst->line_counter)
            {
                show_message(L"index out of bounds", MSG_ERROR);
                return;
            }
        }
        else
            to = inst->line_counter;
    }
    else
    {
        from = 1;
        to = inst->line_counter;
    }

    if (from > to)
        swap_uint(&from, &to);

    skip_whitespaces_tabs();
    if (buffer[buffer_position] == L'^')
        mode = 1, ++buffer_position;
    else if (buffer[buffer_position] == L'$')
        mode = 2, ++buffer_position;
    else
    {
        mode = 3;
        if (buffer[buffer_position] != L'\"')
        {
            show_message(L"pattern must begin with quotes", MSG_ERROR);
            return;
        }

        pattern_capacity = 4;
        pattern = (wchar_t *)malloc(sizeof(wchar_t) * pattern_capacity);
        pattern[0] = WEOS;
        pattern_position = 0;

        ++buffer_position;
        while (1)
        {
            if (buffer[buffer_position] == L'\"')
                break;

            if (buffer[buffer_position] == WEOS)
            {
                show_message(L"closing quotes are not found for pattern", MSG_ERROR);
                free(pattern);
                return;
            }

            if (!(pattern_position + 2 < pattern_capacity))
            {
                pattern_capacity <<= 1;
                wstring_resize(&pattern, pattern_capacity);
            }

            if (buffer[buffer_position] == L'\\')
            {
                int esc_char_type;
                if (buffer[buffer_position + 1] == L'n')
                {
                    show_message(L"pattern must contain only one line", MSG_ERROR);
                    free(pattern);
                    return;
                }
                esc_char_type = escape_character_type(buffer[buffer_position], buffer[buffer_position + 1]);
                if (esc_char_type == -1)
                {
                    show_message(L"unknown escape sequence", MSG_ERROR);
                    free(pattern);
                    return;
                }
                pattern[pattern_position] = escape_characters_replaced[esc_char_type];
                ++buffer_position; /* ok, the next one is proccessed already, skip it :D */
            }
            else
                pattern[pattern_position] = buffer[buffer_position];

            ++buffer_position;
            ++pattern_position;
            pattern[pattern_position] = WEOS;
        }
        if (pattern_position == 0)
        {
            show_message(L"pattern can not be empty", MSG_ERROR);
            free(pattern);
            return;
        }
        ++buffer_position;
    }

    skip_whitespaces_tabs();
    if (buffer[buffer_position] != '\"')
    {
        show_message(L"replacement must begin with quotes", MSG_ERROR);
        if (mode == 3)
            free(pattern);
        return;
    }
    parse_replacement_res = parse_single_quotes(&replacement);
    if (parse_replacement_res != PSQ_SUCCESS)
    {
        switch (parse_replacement_res)
        {
            case PSQ_NO_CLOSING_QUOTE:
                show_message(L"closing quotes are not found", MSG_ERROR);
                break;
            case PSQ_UNKNOWN_ESCAPE_SEQ:
                show_message(L"unknown escape sequence", MSG_ERROR);
                break;
            default:
                show_message(L"unknown error", MSG_ERROR);
                break;
        }
        if (mode == 3)
            free(pattern);
        return;
    }
    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        show_message(L"unexpected characters found after closing quotes", MSG_ERROR);
        if (mode == 3)
            free(pattern);
        return;
    }

    replacement_len = (uint)wcslen(replacement);

    from_line = advance(inst->begin, from - 1);
    to_line = advance(from_line, to - from);

    if (mode == 1 || mode == 2)
    {
        uint i;
        line *current;
        uint accumulator_capacity = 0;

        if (replacement[0] == WEOS)
        {
            free(replacement);
            return;
        }

        for (i = 0, current = from_line; i < 1 + to - from; ++i, current = current->next)
        {
            accumulator_capacity += replacement_len;
            accumulator_capacity += wcslen(current->s);
            accumulator_capacity += 3;
        }
        accumulator = (wchar_t*)malloc(sizeof(wchar_t) * accumulator_capacity);
        accumulator[0] = WEOS;

        for (i = 0, current = from_line; i < 1 + to - from; ++i, current = current->next)
        {
            if (mode == 1)
            {
                wcscat(accumulator, replacement);
                wcscat(accumulator, current->s);
            }
            else
            {
                wcscat(accumulator, current->s);
                wcscat(accumulator, replacement);
            }
            if (i != to - from)
                wcscat(accumulator, L"\n");
            free(current->s);
            current->s = NULL;
        }
        changes_made = 1;
    }
    else if (mode == 3)
    {
        uint **occurences = (uint**)malloc(sizeof(uint*) * (1 + to - from));
        uint *occurences_N = (uint*)malloc(sizeof(uint) * (1 + to - from));

        uint accumulator_position;
        uint replacements_found = 0;
        uint old_size = 0;

        line *current;
        uint i, j;
        pattern_len = (uint)wcslen(pattern);
        for (i = 0, current = from_line; i < 1 + to - from; ++i, current = current->next)
        {
            uint occurences_position = 0;
            wstrstr(current->s, pattern, &occurences[i], &occurences_N[i]);
            for (j = 0; current->s[j] != WEOS; ++j)
            {
                while (occurences_position < occurences_N[i] && j > occurences[i][occurences_position])
                    ++occurences_position;
                if (occurences_position < occurences_N[i] && j == occurences[i][occurences_position])
                {
                    old_size += pattern_len;
                    j += pattern_len; --j;
                    ++replacements_found;
                    continue;
                }
                ++old_size;
            }
        }

        accumulator = (wchar_t*)malloc(sizeof(wchar_t) * (3 + to - from + old_size + replacements_found * replacement_len - replacements_found * pattern_len));
        accumulator[0] = WEOS;
        accumulator_position = 0;
        for (i = 0, current = from_line; i < 1 + to - from; ++i, current = current->next)
        {
            uint occurences_position = 0;
            for (j = 0; current->s[j] != WEOS; ++j)
            {
                while (occurences_position < occurences_N[i] && j > occurences[i][occurences_position])
                    ++occurences_position;
                if (occurences_position < occurences_N[i] && j == occurences[i][occurences_position])
                {
                    wcscat(accumulator + accumulator_position, replacement);
                    j += pattern_len; --j;
                    accumulator_position += replacement_len;
                    ++occurences_position;
                    changes_made = 1;
                    continue;
                }
                accumulator[accumulator_position] = current->s[j];
                accumulator[++accumulator_position] = WEOS;
            }
            if (i != to - from) {
                accumulator[accumulator_position] = WNEWLINE;
                accumulator[++accumulator_position] = WEOS;
            }
            free(current->s);
            current->s = NULL;
        }

        for (i = 0; i < 1 + to - from; ++i)
            free(occurences[i]);
        free(occurences);
        free(occurences_N);
    }

    if (from == 1)
        inst->begin = to_line->next;
    delete_range(from_line, to_line);
    inst->line_counter -= (1 + to - from);

    new_lines = wstring_to_lines(accumulator, &new_lines_length, &new_lines_finish);

    --from;
    if (from == 0)
    {
        insert_lines(new_lines_finish, inst->begin);
        inst->begin = new_lines;
    }
    else
    {
        line *after = advance(inst->begin, from - 1);
        insert_lines(after, new_lines);
    }
    inst->line_counter += new_lines_length;

    free(accumulator);
    free(replacement);
    if (mode == 3)
        free(pattern);
}

void delete_range_handler(instance *inst)
{
    uint from, to;
    line *from_l, *to_l;

    skip_whitespaces_tabs();
    CHECK_WEOS_MISSING
    CHECK_PARSE_UINT(&from)
    skip_whitespaces_tabs();
    if (buffer[buffer_position] == WEOS)
    {
        to = inst->line_counter;
    }
    else
    {
        CHECK_PARSE_UINT(&to)
    }
    skip_whitespaces_tabs();
    CHECK_WEOS_UNEXPECTED

    if (from > to)
        swap_uint(&from, &to);
    if (from == 0 || to == 0 || from > inst->line_counter || to >inst->line_counter)
    {
        show_message(L"index out of bounds", MSG_ERROR);
        return;
    }

    from_l = advance(inst->begin, from - 1);
    to_l = advance(from_l, to - from);
    inst->line_counter -= (to - from + 1);
    if (from == 1)
        inst->begin = to_l->next;
    delete_range(from_l, to_l);

    changes_made = 1;
}

void delete_braces_handler(instance *inst)
{
    uint from, to;

    skip_whitespaces_tabs();
    if (buffer[buffer_position] == WEOS)
    {
        from = 1;
        to = inst->line_counter;
    }
    else
    {
        CHECK_PARSE_UINT(&from)
        skip_whitespaces_tabs();
        if (buffer[buffer_position] == WEOS)
            to = inst->line_counter;
        else
        {
            CHECK_PARSE_UINT(&to);
            skip_whitespaces_tabs();
            CHECK_WEOS_UNEXPECTED
        }
    }

    if (from == 0 || to == 0 || from > inst->line_counter || to > inst->line_counter)
    {
        show_message(L"index out of range", MSG_ERROR);
        return;
    }

    if (from > to)
        swap_uint(&from, &to);

    {
        line *from_l = advance(inst->begin, from - 1);
        line *from_l_prev = from_l->prev;
        line *to_l = advance(from_l, to - from);
        uint range = 1 + to - from;

        int be_result;
        line *be_repl, *be_repl_end;
        uint be_repl_size;

        be_result = bracets_eraser(from_l, range, &be_repl, &be_repl_size, &be_repl_end);
        if (be_result != BE_SUCCESS)
        {
            switch (be_result)
            {
                case BE_WRONGBALANCE:
                    show_message(L"wrong bracets balance", MSG_ERROR);
                    return;
                default:
                    show_message(L"debug me please", MSG_ERROR);
                    return;
            }
        }

        if (from == 1)
            inst->begin = to_l->next;
        inst->line_counter -= range;
        delete_range(from_l, to_l);

        if (from == 1)
        {
            merge_lines(be_repl_end, inst->begin);
            inst->begin = be_repl;
        }
        else
        {
           insert_lines(from_l_prev, be_repl);
        }
        inst->line_counter += be_repl_size;
    }
    changes_made = 1;
    return;
}

int exit_handler(instance *inst)
{
    wchar_t *word;

    skip_whitespaces_tabs();
    if (buffer[buffer_position] == WEOS)
    {
        if (changes_made)
        {
            if (!isatty_stderr)
                fwprintf(stderr, L"%ls\n", "error: file is not saved, use exit force to exit without saving");
            else
                fwprintf(stderr, L"%ls%ls%ls%ls%ls%ls%ls%ls\n",
                         ANSI_COLOR_RED, L"error:", ANSI_COLOR_RESET,
                         L" file is not saved, use ",
                         ANSI_COLOR_GREEN, L"exit force", ANSI_COLOR_RESET,
                         L" to exit without saving");
            return 1;
        }
        else
            return 0;
    }

    parse_word(&word);
    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        show_message(L"unknown command", MSG_ERROR);
        free(word);
        return 1;
    }

    if (wcscmp(word, L"force") == 0)
    {
        skip_whitespaces_tabs();
        if (buffer[buffer_position] != WEOS)
        {
            show_message(L"unknown command", MSG_ERROR);
            free(word);
            return 1;
        }
        free(word);
        return 0;
    }

    show_message(L"unknown command", MSG_ERROR);
    free(word);
    return 1;
}

void read_open_handler(instance **inst, const char remember_association)
{
    FILE *file;
    int pf_res;
    wchar_t *text;
    uint text_capacity;
    uint text_position;

    instance *new_instance;

    char *new_association;

    skip_whitespaces_tabs();
    if (buffer[buffer_position] != L'\"')
    {
        show_message(L"opening quotes are not found", MSG_ERROR);
        return;
    }

    pf_res = parse_filename(&new_association);
    if (pf_res != PF_SUCCESS)
    {
        switch (pf_res)
        {
            case PF_NO_CLOSING_QUOTE:
                show_message(L"closing quotes are not found", MSG_ERROR);
                break;
            case PF_UNKNOWN_SEQUENCE:
                show_message(L"unknown symbols found", MSG_ERROR);
                break;
            default:
                show_message(L"debug me please", MSG_ERROR);
        }
        return;
    }

    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        show_message(L"unexpected arguments found", MSG_ERROR);
        free(new_association);
        return;
    }

    file = fopen(new_association, "r");
    if (!file)
    {
        show_message(L"failed to open file", MSG_ERROR);
        free(new_association);
        return;
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

    destroy_instance(*inst);
    initialize_instance(&new_instance);
    {
        uint lines_cnt;
        line *new_finish;
        new_instance->begin = wstring_to_lines(text, &lines_cnt, &new_finish);
        new_instance->line_counter = lines_cnt;
        changes_made = 0;
    }
    *inst = new_instance;
    free(text);

    if (!remember_association)
    {
        free(new_association);
        return;
    }

    if (association)
        free(association);

    association = new_association;
    return;
}

void write_handler(instance *inst)
{
    FILE *dst;
    char *dst_filename;
    char dst_filename_to_be_erased = 0;

    skip_whitespaces_tabs();
    if (buffer[buffer_position] == WEOS)
    {
        if (!association)
        {
            if (!isatty_stderr)
                fwprintf(stderr, L"%ls\n", "error: specify path to file or set association using set name");
            else
                fwprintf(stderr, L"%ls%ls%ls%ls%ls%ls%ls\n",
                         ANSI_COLOR_RED, L"error:", ANSI_COLOR_RESET,
                         L" specify path to file or set association using ",
                         ANSI_COLOR_GREEN, L"set name", ANSI_COLOR_RESET);
            return;
        }
        dst_filename = association;
    }
    else
    {
        int pf_res;

        skip_whitespaces_tabs();
        if (buffer[buffer_position] != L'\"')
        {
            show_message(L"opening quotes are not found", MSG_ERROR);
            return;
        }

        pf_res = parse_filename(&dst_filename);
        if (pf_res != PF_SUCCESS)
        {
            switch (pf_res)
            {
                case PF_NO_CLOSING_QUOTE:
                    show_message(L"closing quotes are not found", MSG_ERROR);
                    break;
                case PF_UNKNOWN_SEQUENCE:
                    show_message(L"unknown symbols found", MSG_ERROR);
                    break;
                default:
                    show_message(L"debug me please", MSG_ERROR);
            }
            return;
        }

        skip_whitespaces_tabs();
        if (buffer[buffer_position] != WEOS)
        {
            show_message(L"unexpected arguments found", MSG_ERROR);
            free(dst_filename);
            return;
        }
        dst_filename_to_be_erased = 1;
    }

    dst = fopen(dst_filename, "w+");
    if (!dst)
    {
        show_message(L"failed to open file", MSG_ERROR);
        if (dst_filename_to_be_erased)
            free(dst_filename);
        return;
    }

    if (inst->line_counter)
    {
        line *current;
        uint i;
        for (i = 0, current = inst->begin; i < inst->line_counter; ++i, current = current->next)
        {
            if (fwprintf(dst, L"%ls", current->s) < 0 || (current->next && fwprintf(dst, L"\n") < 0))
            {
                show_message(L"unexpected error occurred while writing to file", MSG_ERROR);
                if (dst_filename_to_be_erased)
                    free(dst_filename);
                return;
            }
        }
    }
    fclose(dst);

    if (dst_filename_to_be_erased)
        free(dst_filename);

    changes_made = 0;
}

void set_name_handler(instance *inst)
{
    char *new_association;
    int pf_res;

    skip_whitespaces_tabs();
    if (buffer[buffer_position] != L'\"')
    {
        show_message(L"opening quotes are not found", MSG_ERROR);
        return;
    }
    pf_res = parse_filename(&new_association);
    if (pf_res != PF_SUCCESS)
    {
        switch (pf_res)
        {
            case PF_NO_CLOSING_QUOTE:
                show_message(L"closing quotes are not found", MSG_ERROR);
                break;
            case PF_UNKNOWN_SEQUENCE:
                show_message(L"unknown symbols found", MSG_ERROR);
                break;
            default:
                show_message(L"debug me please", MSG_ERROR);
        }
        return;
    }

    skip_whitespaces_tabs();
    if (buffer[buffer_position] != WEOS)
    {
        show_message(L"unexpected arguments found", MSG_ERROR);
        free(new_association);
        return;
    }

    if (new_association[0] == WEOS)
    {
        if (association != NULL)
        {
            free(association);
            association = NULL;
        }
        free(new_association);
        return;
    }

    if (association != NULL)
        free(association);
    association = new_association;
}
