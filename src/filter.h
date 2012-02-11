#ifndef FILTER_H
#define FILTER_H

#include "env.h"
#include <regex.h>
#include <fts.h>

#define PERM_MASK 0000777

struct filter;

typedef bool (*filter_cmd)(struct filter * f);

enum FilterType {
  NOT_FILTER_ADAPTER,
  FNMATCH_FILTER, 
  REG_FILTER, 
  AMIN_FILTER, 
  TIME_FILTER,
  FILETYPE_FILTER,
  FILESIZE_FILTER,
  USER_FILTER,
  GROUP_FILTER,
  PERM_FILTER
};

typedef struct filter {
  enum FilterType ft;
  filter_cmd cmd;
  struct filter * passed;
  struct filter * failed;
  bool visited;
  void * info;            //some necessary information
} Filter;


bool execute_filter_tree(FTSENT * ent);
void init_filter_tree(int argc, char * argv[]);
void free_filter_tree();

#endif
