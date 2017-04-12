#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#ifndef INSTANCE_H
#define INSTANCE_H

#include "line.h"

extern char config_tabwidth;
extern char config_numbers;
extern char config_wrap;

struct instance_s
{
    line *begin;
    uint line_counter;
};
typedef struct instance_s instance;

void initialize_instance(instance **inst);

void destroy_instance(instance *inst);

#endif
