#include "filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>
#include <fts.h>
#include <fnmatch.h>
#include <unistd.h>
#include <getopt.h>

static Filter filter_tree;

struct stat status;
FTSENT * cur_ent;

Filter * init_filter();
void free_filter_node(Filter * node); 
int exicute_filter(Filter * filter); 
bool reg_filter(Filter * filter);
bool fnmatch_filter(Filter * filter); 


int exicute_filter(Filter * filter) {
  //test whether this filter_tree is passed
  bool filter_result = filter->cmd(filter);
  if (filter_result) {
    if (filter->passed == 0)
      return true;//if no passed return true
    bool exicute_result = exicute_filter(filter->passed);
    if (exicute_result) 
      return true;
  }
  if (filter->failed == 0)
    return false;
  return exicute_filter(filter->failed);
}

bool exicute_filter_tree(FTSENT * ent) {
  stat(ent->fts_path, &status); 
  cur_ent = ent;
  bool passed = exicute_filter(filter_tree.passed);
  return passed;
}

void init_filter_tree(int argc, char * argv[]) {
  int opt;
  Filter * current_filter = &filter_tree;

  struct option longopts[] = {
    {"filename", 1, NULL, 'f'},
    {"regex", 1, NULL, 'r'},
  };

  while((opt = getopt_long(argc, argv, "f:r:", longopts, NULL)) != -1) {
    current_filter->passed = init_filter();
    current_filter = current_filter->passed;
    switch(opt) {
      case 'f':
        fprintf(stderr, "filename is %s\n", optarg);
        init_fnmatch(current_filter, optarg);
        break;
      case 'r':
        fprintf(stderr, "regex is %s\n", optarg);
        init_reg(current_filter, optarg);
        break;
      case ':':
        fprintf(stderr, "option need a value");
        break;
      case '?':
        fprintf(stderr, "unknown option");
        break;
    }
  }
}

/**
 * filter init 
 */
Filter * init_filter() {
  Filter * f = (Filter *) (malloc(sizeof(Filter)));
  f->passed = NULL;
  f->failed = NULL;
  f->cmd = NULL;
  f->info = NULL;
  return f;
}

void _free_filter(Filter * filter) {
  switch(filter->ft) {
    case REG_FILTER:
      free_reg(filter);
      break;
  }
  free(filter);
}

void free_filter_node(Filter * node) {
  if (node == NULL)
    return;
  if (node->passed) {
    free_filter_node(node->passed);
    _free_filter(node->passed);
  }
  if (node->failed) {
    free_filter_node(node->failed);
    _free_filter(node->failed);
  }
}

void free_filter_tree() {
  free_filter_node(&filter_tree); 
}


/**
 * regex filter
 */
void init_reg(Filter * filter, char * pattern) {
  int cflags = 0;
  filter->ft = REG_FILTER;
  filter->cmd = reg_filter;
  filter->info = malloc(sizeof(regex_t));
  regcomp(filter->info, pattern, cflags);
}

void free_reg(Filter * filter) {
  regfree(filter->info);
}

bool reg_filter(Filter * filter) {
  char * filename = cur_ent->fts_name;
  char ebuf[128];
  const size_t nmatch = 1;
  regmatch_t pm[1];
  int r;
  r = regexec(filter->info, filename, nmatch, pm, 0); 
  if (r == REG_NOMATCH)
    return false; 
  return true;
}

/**
 * fnmatch
 */

void init_fnmatch(Filter * filter, char * pattern) {
  filter->info = pattern;
  filter->ft = FNMATCH_FILTER;
  filter->cmd = fnmatch_filter;
}

bool fnmatch_filter(Filter * filter) {
  char * filename = cur_ent->fts_name;
  int ret;
  ret = fnmatch(filter->info, filename, FNM_PATHNAME|FNM_PERIOD);
  if (ret == FNM_NOMATCH)
    return false;
  return true;
}
