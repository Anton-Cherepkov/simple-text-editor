#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#ifndef HELP_H
#define HELP_H

#include "instance.h"
#include "line.h"
#include "parser.h"

#include <wchar.h>

extern const wchar_t *help1;
extern const wchar_t *help2;
extern const wchar_t *help3;
extern const wchar_t *help4;
extern const wchar_t *help5;
extern const wchar_t *help6;
extern const wchar_t *help7;
extern const wchar_t *help8;

void initialize_help_instance(instance **inst);

#endif
