#include "instance.h"

char config_tabwidth = 8;
char config_numbers = 1;
char config_wrap = 0;

void initialize_instance(instance **inst)
{
    *inst = (instance*)malloc(sizeof(instance));
    (*inst)->begin = NULL;
    (*inst)->line_counter = 0;
}

void destroy_instance(instance *inst)
{
    line *current, *temp;

    current = inst->begin;

    while (current)
    {
        temp = current->next;
        destroy_line(current);
        current = temp;
    }
    free(inst);
}
