#ifndef FILTER_H
#define FILTER_H

#include "env.h"
#include <regex.h>
#include <fts.h>

typedef bool (*filter_cmd)();

typedef struct filter {
  filter_cmd cmd;
  struct filter * passed;
  struct filter * failed;
} Filter;


bool exicute_filter_tree(FTSENT * ent);
void init_filter_tree(int argc, char * argv[]);
void free_filter_tree();
void init_reg(char * pattern);
void free_reg();
void init_fnmatch(char * pattern); 

#endif
