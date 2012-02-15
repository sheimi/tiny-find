#ifndef FILTER_H
#define FILTER_H

#include "env.h"
#include <regex.h>
#include <fts.h>

#define PERM_MASK 0000777

struct filter;

typedef bool (*filter_cmd)(struct filter * f); //define function pointer

/* *
 * Here is all the Filter Type Name
 * */
enum FilterType {
  TRUE_FILTER,
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
  enum FilterType ft;     // filer type name
  filter_cmd cmd;         // filter_cmd
  struct filter * passed; // if filter returns true
  struct filter * failed; // if filter returns false
  bool visited;           // a flag to indicate the node is visited
  void * info;            // some necessary information
} Filter;


bool execute_filter_tree(FTSENT * ent);  // to execute the filter tree
void init_filter_tree(int argc);         // to build the filter tree
void free_filter_tree();                 // to free all the memery

#endif
