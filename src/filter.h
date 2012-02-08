#ifndef FILTER_H
#define FILTER_H

#include "env.h"
#include <regex.h>
#include <fts.h>

struct filter;

typedef bool (*filter_cmd)(struct filter * f);

enum FilterType {
  NOT_FILTER_ADAPTER,
  FNMATCH_FILTER, 
  REG_FILTER, 
  AMIN_FILTER, 
  TIME_FILTER
};

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

#endif
