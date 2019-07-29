#ifndef HIGHLEVEL_INIT_H
#define HIGHLEVEL_INIT_H

#include<stdio.h>
#include "typedefs.h"

int generate_malbolge(MemoryList to_initialize, Number entry, FILE* output, size_t line_length);

#endif
