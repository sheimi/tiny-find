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
bool reg_filter();
bool fnmatch_filter(); 


int exicute_filter(Filter * filter) {
  //test whether this filter_tree is passed
  bool filter_result = filter->cmd(&status);
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
        init_fnmatch(optarg);
        current_filter->cmd = fnmatch_filter;
        break;
      case 'r':
        fprintf(stderr, "regex is %s\n", optarg);
        current_filter->cmd = reg_filter; 
        init_reg(optarg);
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
  return f;
}

void free_filter_node(Filter * node) {
  if (node == NULL)
    return;
  if (node->passed) {
    free_filter_node(node->passed);
    free(node->passed);
  }
  if (node->failed) {
    free_filter_node(node->failed);
    free(node->failed);
  }
}

void free_filter_tree() {
  free_filter_node(&filter_tree); 
}


/**
 * regex filter
 */

static regex_t reg;
static bool reg_inited = false; 

void init_reg(char * pattern) {
  int cflags = 0;
  regcomp(&reg, pattern, cflags);
  reg_inited = true;
}

void free_reg() {
  if (reg_inited)
    regfree(&reg);
}

bool reg_filter() {
  char * filename = cur_ent->fts_name;
  char ebuf[128];
  const size_t nmatch = 1;
  regmatch_t pm[1];
  int r;
  r = regexec(&reg, filename, nmatch, pm, 0); 
  if (r == REG_NOMATCH)
    return false; 
  return true;
}

/**
 * fnmatch
 */
static char * fnmatch_pattern; 

void init_fnmatch(char * pattern) {
  fnmatch_pattern = pattern;
}

bool fnmatch_filter() {
  char * filename = cur_ent->fts_name;
  int ret;
  ret = fnmatch(fnmatch_pattern, filename, FNM_PATHNAME|FNM_PERIOD);
  if (ret == FNM_NOMATCH)
    return false;
  return true;
}
