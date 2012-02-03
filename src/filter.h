#ifndef FILTER_H
#define FILTER_H

#include "env.h"
#include <sys/stat.h>

typedef bool (*filter_cmd)(struct stat * status);

typedef struct filter {
  filter_cmd cmd;
  struct filter * passed;
  struct filter * failed;
} Filter;

extern Filter * filter_tree;
bool exicute_filter_tree(char * path);
void init_test();
void free_filter_tree();

#endif
