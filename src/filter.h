#ifndef FILTER_H
#define FILTER_H

#include "env.h"
#include <regex.h>
#include <fts.h>

struct filter;

typedef bool (*filter_cmd)(struct filter * f);

enum FilterType {FNMATCH_FILTER, REG_FILTER};

typedef struct filter {
  enum FilterType ft;
  filter_cmd cmd;
  struct filter * passed;
  struct filter * failed;
  void * info;            //some necessary information
} Filter;


bool exicute_filter_tree(FTSENT * ent);
void init_filter_tree(int argc, char * argv[]);
void free_filter_tree();
void init_reg(Filter * filter, char * pattern);
void free_reg(Filter * filter);
void init_fnmatch(Filter * filter, char * pattern); 

#endif
