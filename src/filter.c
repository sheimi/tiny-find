#include "filter.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>
#include <fts.h>
#include <fnmatch.h>
#include <unistd.h>
#include <time.h>

static Filter filter_tree;

struct stat status;
FTSENT * cur_ent;

static Filter ** filter_list;
static int filter_list_size;
static int filter_list_len;

enum time_type {ATIME, AMIN, CTIME, CMIN};

Filter * init_filter();
int execute_filter(Filter * filter); 
void init_visit_filter();

void connect_filter(Filter * first, Filter * second);
void filter_and();
void filter_or(); 
void filter_not();

//filters
void init_reg(char * pattern);
bool reg_filter(Filter * filter);

void init_fnmatch(char * pattern); 
bool fnmatch_filter(Filter * filter); 

void init_time(enum time_type tt, char * pattern);
bool time_filter(Filter * filter);

void init_not_filter_adapter(Filter * filter);
bool not_filter_adapter(Filter * filter);
void set_not_adapter_info (Filter * filter, Filter * inner_filter); 


void init_visit_filter() {
  int i;
  for (i = 0; i < filter_list_len; i++) {
    filter_list[i]->visited = false;
  }
}

int execute_filter(Filter * filter) {
  //test whether this filter_tree is passed
  bool filter_result = filter->cmd(filter);
  if (filter_result) {
    if (filter->passed == 0)
      return true;//if no passed return true
    bool execute_result = execute_filter(filter->passed);
    if (execute_result) 
      return true;
  }
  if (filter->failed == 0)
    return false;
  return execute_filter(filter->failed);
}

bool execute_filter_tree(FTSENT * ent) {
  stat(ent->fts_path, &status); 
  cur_ent = ent;
  bool passed = execute_filter(filter_tree.passed);
  return passed;
}


/*init helper*/
static Filter * op_stack[2];
static int op_stack_len;

void push_op_stack(Filter * op) {
  if (op_stack_len == 2) {
    filter_and();
    op_stack[1] = op;
    return;
  }
  op_stack[op_stack_len] = op;
  op_stack_len++;
}

void connect_filter(Filter * first, Filter * second) {
  if (first == NULL || first->visited) {
    return;
  }
  first->visited = true;
  if (first->passed == NULL) {
    first->passed = second; 
    return;
  }
  connect_filter(first->passed, second);
  connect_filter(first->failed, second);
}


void filter_and() {
  int i;
  Filter * first = op_stack[0];
  Filter * second = op_stack[1];
  op_stack_len = 1;
  if (first->passed == NULL) {
    first->passed = second;
    return;
  }
  if (second->passed == NULL) {
    second->passed = first;
    op_stack[0] = second;
    return;
  }
  for (i = 0; i < filter_list_len; i++) {
    connect_filter(first->passed, second);
  }
}

void filter_or() {
  Filter * first = op_stack[0];
  Filter * second = op_stack[1];
  op_stack_len = 1;
  Filter * now = first;
  while(now->failed != NULL) {
    now = now->failed;
  }
  now->failed = second;
}


void filter_not() {
  Filter * first = op_stack[op_stack_len - 1];
  Filter * nfa = init_filter();
  init_not_filter_adapter(nfa);
  set_not_adapter_info(nfa, first);
  op_stack[op_stack_len - 1] = nfa;
}


void init_filter_tree(int argc, char * argv[]) {
  char * exp;
  init_parser(argc, argv);

  filter_list_size = argc;
  filter_list_len = 0;
  filter_list = (Filter **)(malloc(sizeof(Filter) * argc));

  while((exp = get_exp()) != NULL) {
    char * optarg;
    if (IS_EQUAL(exp, "-name")) {
      optarg = get_exp();
      fprintf(stderr, "filename is %s\n", optarg);
      init_fnmatch(optarg);
    } else if (IS_EQUAL(exp, "-regex")) {
      optarg = get_exp();
      fprintf(stderr, "regex is %s\n", optarg);
      init_reg(optarg);
    } else if (IS_EQUAL(exp, "-amin")) {
      optarg = get_exp();
      fprintf(stderr, "min deltais %s\n", optarg);
      init_time(AMIN, optarg);
    } else if (IS_EQUAL(exp, "-atime")) {
      optarg = get_exp();
      fprintf(stderr, "day delta is %s\n", optarg);
      init_time(ATIME, optarg);
    } else if (IS_EQUAL(exp, "-cmin")) {
      optarg = get_exp();
      fprintf(stderr, "min deltais %s\n", optarg);
      init_time(CMIN, optarg);
    } else if (IS_EQUAL(exp, "-ctime")) {
      optarg = get_exp();
      fprintf(stderr, "day delta is %s\n", optarg);
      init_time(CTIME, optarg);
    } else if (IS_EQUAL(exp, "-not")) {
      fprintf(stderr, "filter not adapter\n");
      filter_not();
    } else if (IS_EQUAL(exp, "-and")) {
      fprintf(stderr, "filter and\n");
      filter_and();
    } else if (IS_EQUAL(exp, "-or")) {
      fprintf(stderr, "filter or\n");
      filter_or();
    } else {
      //TODO
    }
  }
  if (op_stack_len == 2)
    filter_and();
  filter_tree.passed = op_stack[0];
  free_parser();
}

/**
 * filter init 
 */
Filter * init_filter() {
  Filter * f = (Filter *) (malloc(sizeof(Filter)));
  filter_list[filter_list_len] = f;
  filter_list_len++;
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
  }
  free(filter);
}

void free_filter_tree() {
  int i;
  for(i = 0; i < filter_list_len; i++) {
    _free_filter(filter_list[i]);
  } 
  free(filter_list);
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
  return !execute_filter(inner_filter);
}


/**
 * regex filter
 */
void init_reg(char * pattern) {
  int cflags = 0;
  Filter * filter = init_filter();
  filter->ft = REG_FILTER;
  filter->cmd = reg_filter;
  filter->info = malloc(sizeof(regex_t));
  regcomp(filter->info, pattern, cflags);
  push_op_stack(filter);
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

void init_fnmatch(char * pattern) {
  Filter * filter = init_filter();  
  filter->info = pattern;
  filter->ft = FNMATCH_FILTER;
  filter->cmd = fnmatch_filter;
  push_op_stack(filter);
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

void init_time(enum time_type tt, char * pattern) {
  Filter * filter = init_filter();  
  struct time_info * ti = malloc(sizeof(struct time_info));
  ti->tt = tt;
  ti->value = pattern;
  filter->info = ti;
  filter->ft = TIME_FILTER;
  filter->cmd = time_filter;
  push_op_stack(filter);
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


