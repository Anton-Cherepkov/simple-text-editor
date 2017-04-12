#include "parser.h"

const wchar_t escape_characters[] = {L'n', L't', L'r', L'\\'};
const wchar_t escape_characters_replaced[] = {L'\n', L'\t', L'\r', L'\\'};

int escape_character_type(wchar_t c1, wchar_t c2)
{
    uint i;
    for (i = 0; i < 4; ++i)
        if (c1 == L'\\' && c2 == escape_characters[i])
            return i;
    return -1;
}

void wstring_resize(wchar_t **wstring, uint new_size)
{
    wchar_t *temp = (wchar_t*)malloc(sizeof(wchar_t) * new_size);
    uint i;

    for (i = 0; i < new_size && (*wstring)[i] != WEOS; ++i)
        temp[i] = (*wstring)[i];

    temp[MIN(i, new_size - 1)] = WEOS;

    free(*wstring);
    *wstring = temp;
}

void wstring_substr(wchar_t *wstring, wchar_t **result, uint position, uint size)
{
    uint i, j;
    *result = (wchar_t*)malloc(sizeof(wchar_t) * (size + 2));
    for (i = position, j = 0; j < size; ++i, ++j)
        (*result)[j] = wstring[i];
    (*result)[size] = WEOS;
}

void wstring_split(wchar_t *wstring, wchar_t **left, wchar_t **right, uint left_size)
{
    if (left_size == 0)
    {
        *left = (wchar_t*)malloc(sizeof(wchar_t) * 2);
        (*left)[0] = WEOS;
        *right = wstring;
        return;
    }
    if (left_size == wcslen(wstring))
    {
        *right = (wchar_t*)malloc(sizeof(wchar_t) * 2);
        (*right)[0] = WEOS;
        *left = wstring;
        return;
    }
    wstring_substr(wstring, left, 0, left_size);
    wstring_substr(wstring, right, left_size, (uint)wcslen(wstring) - left_size);
    free(wstring);
}

void wstring_merge(wchar_t **wstring, wchar_t *left, wchar_t *right)
{
    *wstring = (wchar_t*)malloc(sizeof(wchar_t) * (wcslen(left) + wcslen(right) + 2));
    (*wstring)[0] = WEOS;
    wcscat(*wstring, left);
    wcscat(*wstring, right);
    free(left);
    free(right);
}

void initialize_buffer()
{
    buffer = (wchar_t*)malloc(sizeof(wchar_t) * INITIAL_BUFFER_CAPACITY);
    buffer_capacity = INITIAL_BUFFER_CAPACITY;
    buffer_position = 0;
}

void destroy_buffer()
{
    free(buffer);
}

int read_line_to_buffer()
{
    wchar_t current;
    buffer_position = 0;

    for (current = fgetwc(stdin); current != WEOF && current != WNEWLINE; current = fgetwc(stdin), ++buffer_position)
    {
        if (!(buffer_position < buffer_capacity - 1))
        {
            buffer_capacity <<= 1;
            wstring_resize(&buffer, buffer_capacity);
        }
        buffer[buffer_position] = current;
    }
    buffer[buffer_position] = WEOS;
    if (buffer_position == 0 && current == WEOF)
        return -1;
    buffer_position = 0;
    return 0;
}

void skip_whitespaces_tabs()
{
    while (buffer[buffer_position] == WSPACE || buffer[buffer_position] == WTAB)
        ++buffer_position;
    if (buffer[buffer_position] == L'#')
        buffer[buffer_position] = WEOS;
}

int parse_wchar(wchar_t *result)
{
    if (buffer[buffer_position] == WEOS)
        return -1;
    *result = buffer[buffer_position];
    ++buffer_position;
    return 0;
}

int parse_word(wchar_t **result)
{
    wchar_t *temp;
    uint temp_capacity;
    uint temp_size;

    if (buffer[buffer_position] == WEOS)
        return -1;

    temp = (wchar_t*)malloc(sizeof(wchar_t) * 2);
    temp[0] = L'\0';
    temp_capacity = 2;
    temp_size = 0;

    while (!(buffer[buffer_position] == WSPACE || buffer[buffer_position] == WTAB) && buffer[buffer_position] != WEOS)
    {
        if (!(temp_size < temp_capacity - 1))
        {
            temp_capacity <<= 1;
            wstring_resize(&temp, temp_capacity);
        }
        if (buffer[buffer_position] == L'#')
        {
            buffer[buffer_position] = WEOS;
            break;
        }
        temp[temp_size] = buffer[buffer_position];
        temp[temp_size + 1] = L'\0';
        ++temp_size;
        ++buffer_position;
    }
    *result = temp;
    return 0;
}

int parse_uint(uint *result)
{
    if (buffer[buffer_position] == WEOS)
        return -1;
    *result = 0;
    while (!(buffer[buffer_position] == WSPACE || buffer[buffer_position] == WTAB) && buffer[buffer_position] != WEOS) {
        if (buffer[buffer_position] == L'#')
        {
            buffer[buffer_position] = WEOS;
            break;
        }
        if (!(buffer[buffer_position] >= L'0' && buffer[buffer_position] <= L'9'))
            return -1;
        *result *= 10;
        *result += (buffer[buffer_position] - L'0');
        ++buffer_position;
    }
    return 0;
}

int parse_single_quotes(wchar_t **result)
{
    int err_no;
    int escape_char_type;
    uint result_capacity = 2;
    uint result_position = 0;

    assert(buffer[buffer_position] == L'\"');

    *result = (wchar_t*)malloc(sizeof(wchar_t) * result_capacity);
    (*result)[0] = WEOS;

    ++buffer_position;
    while (1)
    {
        if (buffer[buffer_position] == WEOS)
        {
            err_no = PSQ_NO_CLOSING_QUOTE;
            goto error;
        }

        if (buffer[buffer_position] == L'\"')
            break;

        if (!(result_position + 1 < result_capacity))
        {
            result_capacity <<= 1;
            wstring_resize(result, result_capacity);

        }

        if (buffer[buffer_position] == L'\\')
        {
            if (buffer[buffer_position + 1] == L'\"')
            {
                (*result)[result_position] = L'\"';
                ++buffer_position; /* let's skip the next symbol :) */
            }
            else
            {
                escape_char_type = escape_character_type(buffer[buffer_position], buffer[buffer_position + 1]);
                if (escape_char_type == -1)
                {
                    err_no = PSQ_UNKNOWN_ESCAPE_SEQ;
                    goto error;
                }
                else
                {
                    (*result)[result_position] = escape_characters_replaced[escape_char_type];
                    ++buffer_position; /* let's skip the next symbol :) */
                }
            }
        }
        else
        {
            (*result)[result_position] = buffer[buffer_position];
        }

        (*result)[++result_position] = WEOS;
        ++buffer_position;
    }
    ++buffer_position;

    return PSQ_SUCCESS;

    error: free(*result);
    return err_no;
}

int parse_filename(char **result)
{
    wchar_t *temp;
    uint temp_capacity;
    uint temp_position;

    assert(buffer[buffer_position] == '\"');
    ++buffer_position;

    temp_capacity = 4;
    temp = (wchar_t*)malloc(sizeof(wchar_t) * temp_capacity);
    temp[0] = WEOS;
    temp_position = 0;

    while (1)
    {
        if (buffer[buffer_position] == WEOS)
        {
            free(temp);
            ++buffer_position;
            return PF_NO_CLOSING_QUOTE;
        }

        if (buffer[buffer_position] == L'\"')
        {
            ++buffer_position;
            break;
        }

        if (!(temp_position + 2 < temp_capacity))
        {
            temp_capacity <<= 1;
            wstring_resize(&temp, temp_capacity);
        }

        temp[temp_position] = buffer[buffer_position];
        temp[++temp_position] = WEOS;
        ++buffer_position;
    }

    *result = (char*)malloc(sizeof(wchar_t) * (wcslen(temp) + 3));
    if (wcstombs(*result, temp, sizeof(wchar_t) * (wcslen(temp) + 3)) == (size_t) -1)
    {
        free(temp);
        free(*result);
        return PF_UNKNOWN_SEQUENCE;
    }
    free(temp);
    return PF_SUCCESS;
}

void wstrstr(wchar_t *text, wchar_t *pattern, uint **result, uint *counter)
{
    wchar_t *pattern_text;
    uint *z_function;
    uint text_len;
    uint pattern_len;
    uint pattern_text_len;

    text_len = (uint)wcslen(text), pattern_len = (uint)(wcslen(pattern));
    pattern_text_len = text_len + pattern_len + 1;

    pattern_text = (wchar_t*)malloc(sizeof(wchar_t) * (pattern_text_len + 2));
    pattern_text[0] = WEOS;
    wcscat(pattern_text, pattern);
    pattern_text[pattern_len + 1] = WEOS;
    wcscat(pattern_text + pattern_len + 1, text);

    {
        uint i, l, r;

        z_function = (uint *) malloc(sizeof(uint) * (pattern_text_len));
        for (i = 0; i < pattern_text_len; ++i)
            z_function[i] = 0;
        for (i = 0, l = 0, r = 0; i < pattern_text_len; ++i) {
            if (i <= r)
                z_function[i] = MIN(r - i + 1, z_function[i - l]);
            while (i + z_function[i] < pattern_text_len &&
                   pattern_text[z_function[i]] == pattern_text[i + z_function[i]])
                ++z_function[i];
            if (i + z_function[i] - 1 > r)
                l = i, r = i + z_function[i] - 1;
        }
    }

    {
        uint i;
        uint result_position = 0;
        uint cnt = 0;

        for (i = 1; i < pattern_text_len; ++i)
            if (z_function[i] == pattern_len)
                ++cnt;

        *result = (uint*)malloc(sizeof(uint) * cnt);
        *counter = cnt;
        for (i = 1, result_position = 0; i < pattern_text_len; ++i)
            if (z_function[i] == pattern_len)
            {
                (*result)[result_position] = i - pattern_len - 1;
                ++result_position;
            }
    }

    free(z_function);
    free(pattern_text);
}
