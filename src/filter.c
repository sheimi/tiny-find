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
#include <time.h>

static Filter filter_tree;

struct stat status;
FTSENT * cur_ent;

enum time_type {ATIME, AMIN, CTIME, CMIN};

Filter * init_filter();
void free_filter_node(Filter * node); 
int exicute_filter(Filter * filter); 
//filters
void init_reg(Filter * filter, char * pattern);
bool reg_filter(Filter * filter);

void init_fnmatch(Filter * filter, char * pattern); 
bool fnmatch_filter(Filter * filter); 

void init_time(Filter * filter, enum time_type tt, char * pattern);
bool time_filter(Filter * filter);

void init_not_filter_adapter(Filter * filter);
bool not_filter_adapter(Filter * filter);
void set_not_adapter_info (Filter * filter, Filter * inner_filter); 

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
  int not_enable = -1;
  Filter * now = &filter_tree;

  struct option longopts[] = {
    {"not", 0, NULL, 'n'}, 
    {"name", 1, NULL, 'a'},
    {"regex", 1, NULL, 'r'},
    {"amin", 1, NULL, 'm'},
    {"atime", 1, NULL, 't'},
    {"cmin", 1, NULL, 'i'},
    {"ctime", 1, NULL, 'e'},
  };

  while((opt = getopt_long(argc, argv, "na:r:m:t:", 
                           longopts, NULL)) != -1) {
    Filter * current_filter = init_filter();
    switch(opt) {
      case 'n':
        fprintf(stderr, "filter not adapter\n");
        not_enable = 2;
        init_not_filter_adapter(current_filter);
        break;
      case 'a':
        fprintf(stderr, "filename is %s\n", optarg);
        init_fnmatch(current_filter, optarg);
        break;
      case 'r':
        fprintf(stderr, "regex is %s\n", optarg);
        init_reg(current_filter, optarg);
        break;
      case 'm':
        fprintf(stderr, "min deltais %s\n", optarg);
        init_time(current_filter, AMIN, optarg);
        break;
      case 't':
        fprintf(stderr, "day delta is %s\n", optarg);
        init_time(current_filter, ATIME, optarg);
        break;
      case 'i':
        fprintf(stderr, "min deltais %s\n", optarg);
        init_time(current_filter, CMIN, optarg);
        break;
      case 'e':
        fprintf(stderr, "day delta is %s\n", optarg);
        init_time(current_filter, CTIME, optarg);
        break;
      case ':':
        fprintf(stderr, "option need a value");
        break;
      case '?':
        fprintf(stderr, "unknown option");
        break;
    }
    not_enable--;
    if (not_enable == 0) {
      set_not_adapter_info(now, current_filter); 
      not_enable = false;
    } else {
      now->passed = current_filter;
      now = now->passed;
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
      regfree(filter->info);
      free(filter->info);
      break;
    case TIME_FILTER:
      free(filter->info);
      break;
    case NOT_FILTER_ADAPTER:
      _free_filter(filter->info);
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

void init_not_filter_adapter(Filter * filter) {
  filter->ft = NOT_FILTER_ADAPTER;
  filter->cmd = not_filter_adapter;
}

void set_not_adapter_info (Filter * filter, Filter * inner_filter) {
  filter->info = inner_filter;
}

bool not_filter_adapter(Filter * filter) {
  Filter * inner_filter = (Filter *) filter->info;
  return !inner_filter->cmd(inner_filter);
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

bool reg_filter(Filter * filter) {
  char * filename = cur_ent->fts_name;
  //char ebuf[128];
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

/* *
 * time filter
 * */

struct time_info {
  enum time_type tt;
  char * value; 
};

void init_time(Filter * filter, enum time_type tt, char * pattern) {
  struct time_info * ti = malloc(sizeof(struct time_info));
  ti->tt = tt;
  ti->value = pattern;
  filter->info = ti;
  filter->ft = TIME_FILTER;
  filter->cmd = time_filter;
}

bool time_filter(Filter * filter) {
  struct time_info * ti = filter->info;
  enum time_type tt = ti->tt;
  int t_del = atoi(ti->value);

  struct stat buf;
  time_t cur_time;
  double diff;

  bool result = false;

  cur_time = time(NULL);
  stat(cur_ent->fts_path, &buf);
  switch(tt) {
    case ATIME:
      diff = difftime(cur_time, buf.st_atime);
      result = diff < t_del * 60 * 60 * 24;
      break;
    case AMIN:
      diff = difftime(cur_time, buf.st_atime);
      result = diff < t_del * 60;
      break;
    case CTIME:
      diff = difftime(cur_time, buf.st_ctime);
      result = diff < t_del * 60 * 60 * 24;
      break;
    case CMIN:
      diff = difftime(cur_time, buf.st_ctime);
      result = diff < t_del * 60;
      break;
  }
  return result;
}
