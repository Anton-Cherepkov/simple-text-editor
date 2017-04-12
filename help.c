#include "help.h"

const wchar_t *help1 = L"[::::Текстовый  редактор::::]\n[Разработчик: Черепков Антон]\n[ anthonycherepkov@gmail.com]\n[     + 7 911 866 52 **     ]\n\n";
const wchar_t *help2 = L"I. Команды\n\nПросмотр текста:\n set tabwidth N - заменять каждый знак табуляции на N пробелов в интерактивном режиме.\n set numbers (\'yes\' | \'no\') - вкл./выкл. отображение номеров строк в интерактивном режиме.\n set wrap (\'yes\' | \'no\') - вкл./выкл. режим «заворачивания» текста в интерактивном режиме.";
const wchar_t *help3 = L" print range FROM [TO] - включить интерактивный режим просмотра строк из диапазона [FROM; TO]. Если параметр TO не указан, то просмотр до конца файла.\n  Управление в интерактивном режиме осуществляется следующими клавишами:\n  ←, →, ↑, ↓ - листать текст (не доступны в режиме «заворачивания»)\n  SPACE - перелистнуть страницу\n  q - выйти из интерактивного режима\n print pages - включить интерактивный режим просмотра всего текста.\n";
const wchar_t *help4 = L"\nВставка строк:\n insert after [N] \"TEXT\" - вставить TEXT после строки с номером N. Могут быть экранированы следующие символы: \'\\n\', \'\\', \'\\r\', \'\\\\\', \'\\\"\'.\n insert after [N] \"\"\"\nTEXT\"\"\" - команда, аналогичная предыдущей, но TEXT может быть многострочным.\n\n";
const wchar_t *help5 = L"Редактирование строк:\n edit string N M SYMBOL - изменить в N-ой строке M-ый символ на SYMBOL. Могут быть экранированы следующие символы: \'\\n\', \'\\', \'\\r\', \'\\\\\', \'\\#\'.\n insert symbol N M SYMBOL - вставить в N-ой строке в M-ую позицию символ SYMBOL.\n replace substring [FROM [TO]] \"PATTERN\" \"REPLACEMENT\" - заменить в строчках из диапазона [FROM; TO] шаблон PATTERN на REPLACEMENT.\n replace substring [FROM [TO]] ^ \"TEXT\" - вставить TEXT в начало строк из диапазона [FROM; TO].";
const wchar_t *help6 = L" replace substring [FROM [TO]] $ \"TEXT\" - вставить TEXT в конец строк из диапазона [FROM; TO].\n delete range FROM [TO] - удалить строки из диапазона [FROM; TO].\n delete braces [FROM [TO]] - удаляет текст, заключённый в фигурные скобки в диапазоне [FROM; TO].\n\n";
const wchar_t *help7 = L"Технические команды:\n exit - выйти из текстового редактора.\n read PATH - прочитать содержимое файла PATH.\n open PATH - аналогично read. PATH запоминается для последующих сохранений в этот файл.\n write [PATH] - запись в файл. Если PATH не указан, то запись производится в текущий файл.\n set name PATH - запомнить PATH для последующих сохранений в этот файл.\n\n";
const wchar_t *help8 = L"II. Прочее\n\nВ командах могуть быть указаны комментарии, начинающиеся с символа \'#\'. Всё, следующее за ним, будет проигнорировано. Если \'#\' встречается в вставляемом тексте, то комментарием это не является.\n\nВ аргументы программы может быть передан PATH файла, заключенный в двойные кавычки, который необходимо открыть. Иначе создаётся пустое множество символов.";

void initialize_help_instance(instance **inst)
{
    uint sz1, sz2, sz3, sz4, sz5, sz6, sz7, sz8;
    line *l1, *l2, *l3, *l4, *l5, *l6, *l7, *l8;
    line *eol1, *eol2, *eol3, *eol4, *eol5, *eol6, *eol7, *eol8;
    initialize_instance(inst);
    l1 = wstring_to_lines(help1, &sz1, &eol1);
    l2 = wstring_to_lines(help2, &sz2, &eol2);
    l3 = wstring_to_lines(help3, &sz3, &eol3);
    l4 = wstring_to_lines(help4, &sz4, &eol4);
    l5 = wstring_to_lines(help5, &sz5, &eol5);
    l6 = wstring_to_lines(help6, &sz6, &eol6);
    l7 = wstring_to_lines(help7, &sz7, &eol7);
    l8 = wstring_to_lines(help8, &sz8, &eol8);

    merge_lines(eol1, l2);
    merge_lines(eol2, l3);
    merge_lines(eol3, l4);
    merge_lines(eol4, l5);
    merge_lines(eol5, l6);
    merge_lines(eol6, l7);
    merge_lines(eol7, l8);

    (*inst)->begin = l1;
    (*inst)->line_counter = sz1 + sz2 + sz3 + sz4 + sz5 + sz6 + sz7 + sz8;
}
