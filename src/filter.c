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
#include <string.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>

#include "parser.h"
#include "filter.h"

static Filter filter_tree;        // whether the filter is filter

static struct stat status;               // the file status of current file
static FTSENT * cur_ent;                 // the FTSENT of current file

static Filter ** filter_list;
static int filter_list_size;
static int filter_list_len;

/* *
 *  for time filter
 * */
enum time_type {ATIME, AMIN, ANEWER, CTIME, CMIN, CNEWER, MTIME, MMIN, MNEWER};

struct time_info {
  enum time_type tt;
  void * value; 
};

/**
 * DECLARATION OF FUNCTIONS
 **/
static Filter * init_filter();
static int execute_filter(Filter * filter); 
static void init_visit_filter();

static void free_filter(Filter * filter);
static void connect_filter(Filter * first, Filter * second);
static void filter_and();
static void filter_or(); 
static void filter_not();

static void init_not_filter_adapter(Filter * filter);
static bool not_filter_adapter(Filter * filter);
static void set_not_adapter_info (Filter * filter, Filter * inner_filter); 

//filters
static bool true_filter(Filter * filter);
static void init_true(); 

static void init_reg(char * pattern);
static bool reg_filter(Filter * filter);

static void init_fnmatch(char * pattern, bool case_s); 
static bool fnmatch_filter(Filter * filter); 

static void init_time(enum time_type tt, char * pattern);
static bool time_filter(Filter * filter);

static void init_filetype(char * ft);
static bool filetype_filter(Filter * filter);

static void init_filesize(char * size);
static bool filesize_filter(Filter * filter);

static void init_user(char * name);
static bool user_filter(Filter * filter);

static void init_group(char * name);
static bool group_filter(Filter * filter);

static void init_perm(char * name);
static bool perm_filter(Filter * filter);


/*
 * the interface, not static
 */
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
#ifdef DEBUG
  fprintf(stderr, "file level: %d\n", ent->fts_level);
#endif
  switch(options.symbol_handle) {
    case S_L:
      stat(ent->fts_path, &status); 
      break;
    case S_P:
      lstat(ent->fts_path, &status); 
      break;
    case S_H:
      if (ent->fts_level == 0) {
        stat(ent->fts_path, &status); 
      } else {
        lstat(ent->fts_path, &status); 
      }
      break;
  }
  cur_ent = ent;
  bool passed = execute_filter(filter_tree.passed);
  return passed;
}

void free_filter_tree() {
  int i;
  for(i = 0; i < filter_list_len; i++) {
    free_filter(filter_list[i]);
  } 
  free(filter_list);
}


/* *
 * init filters
 * */
static Filter ** op_stack;
static int op_stack_len;

static void init_op_stack(int size) {
  op_stack = (Filter **)(malloc(sizeof(Filter *) * size));
  op_stack_len = 0;
}

static void free_op_stack() {
  free(op_stack);
}

static void init_visit_filter() {
  int i;
  for (i = 0; i < filter_list_len; i++) {
    filter_list[i]->visited = false;
  }
}

static void push_op_stack(Filter * op) {
  op_stack[op_stack_len] = op;
  op_stack_len++;
}

static Filter * pop_op_stack() {
  op_stack_len--;
  return op_stack[op_stack_len];
}

static void connect_filter(Filter * first, Filter * second) {
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

static void filter_and() {
  Filter * first = pop_op_stack();
  Filter * second = pop_op_stack();
  if (first->passed == NULL && first->failed == NULL) {
    first->passed = second;
    push_op_stack(first);
    return;
  }
  if (second->passed == NULL && second->failed == NULL) {
    second->passed = first;
    push_op_stack(second);
    return;
  }
  init_visit_filter();
  connect_filter(first, second);
  push_op_stack(first);
}

static void filter_or() {
  Filter * first = pop_op_stack();
  Filter * second = pop_op_stack();
  Filter * now = first;
  while(now->failed != NULL) {
    now = now->failed;
  }
  now->failed = second;
  push_op_stack(first);
}


static void filter_not() {
  Filter * first = pop_op_stack();
  Filter * nfa = init_filter();
  init_not_filter_adapter(nfa);
  set_not_adapter_info(nfa, first);
  push_op_stack(nfa);
}


/* *
 * helper function to get the args
 * */
static char * _get_arg(char * name) {
  char * exp = get_exp();
  if (exp == NULL) {
    require_arg(name);
  } 
#ifdef DEBUG
  fprintf(stderr, "expression is %s\n", name);
  fprintf(stderr, "arg is %s\n", exp);
#endif
  return exp;
}

/*
 * interface, not static 
 */
void init_filter_tree(int argc) {
  char * exp;
  int exp_num = 0;
  
  init_op_stack(argc);


  filter_list_size = argc;
  filter_list_len = 0;
  filter_list = (Filter **)(malloc(sizeof(Filter) * argc));

  while((exp = get_exp()) != NULL) {
    exp_num++;
    char * optarg;
    //all the exps
    if (IS_EQUAL(exp, "-name")) {
      optarg = _get_arg("-name");
      init_fnmatch(optarg, true);
    } else if (IS_EQUAL(exp, "-iname")) {
      optarg = _get_arg("-iname");
      init_fnmatch(optarg, false);
    } else if (IS_EQUAL(exp, "-user")) {
      optarg = _get_arg("-user");
      init_user(optarg);
    } else if (IS_EQUAL(exp, "-group")) {
      optarg = _get_arg("-group");
      init_group(optarg);
    } else if (IS_EQUAL(exp, "-perm")) {
      optarg = _get_arg("-perm");
      init_perm(optarg);
    } else if (IS_EQUAL(exp, "-regex")) {
      optarg = _get_arg("-regex");
      init_reg(optarg);
    } else if (IS_EQUAL(exp, "-amin")) {
      optarg = _get_arg("-amin");
      init_time(AMIN, optarg);
    } else if (IS_EQUAL(exp, "-atime")) {
      optarg = _get_arg("-atime");
      init_time(ATIME, optarg);
    } else if (IS_EQUAL(exp, "-anewer")) {
      optarg = _get_arg("-anewer");
      init_time(ANEWER, optarg);
    } else if (IS_EQUAL(exp, "-cnewer")) {
      optarg = _get_arg("-cnewer");
      init_time(CNEWER, optarg);
    } else if (IS_EQUAL(exp, "-cmin")) {
      optarg = _get_arg("-cmin");
      init_time(CMIN, optarg);
    } else if (IS_EQUAL(exp, "-ctime")) {
      optarg = _get_arg("-ctime");
      init_time(CTIME, optarg);
    } else if (IS_EQUAL(exp, "-mtime")) {
      optarg = _get_arg("-mtime");
      init_time(MMIN, optarg);
    } else if (IS_EQUAL(exp, "-mnewer")) {
      optarg = _get_arg("-mnewer");
      init_time(MNEWER, optarg);
    } else if (IS_EQUAL(exp, "-type")) {
      optarg = _get_arg("-type");
      init_filetype(optarg);
    } else if (IS_EQUAL(exp, "-size")) {
      optarg = _get_arg("-size");
      init_filesize(optarg);
    } else if (IS_EQUAL(exp, "-not")) {
#ifdef DEBUG
      fprintf(stderr, "filter not adapter\n");
#endif
      filter_not();
    } else if (IS_EQUAL(exp, "-and")) {
#ifdef DEBUG
      fprintf(stderr, "filter and\n");
#endif
      filter_and();
    } else if (IS_EQUAL(exp, "-or")) {
#ifdef DEBUG
      fprintf(stderr, "filter or\n");
#endif
      filter_or();
    } else {
      //TODO
    }
  }
  if (exp_num == 0) {
    init_true(); 
  }
  filter_tree.passed = op_stack[0];
  free_op_stack();
}

/**
 * filter init 
 */
static Filter * init_filter() {
  Filter * f = (Filter *) (malloc(sizeof(Filter)));
  filter_list[filter_list_len] = f;
  filter_list_len++;
  f->passed = NULL;
  f->failed = NULL;
  f->cmd = NULL;
  f->info = NULL;
  return f;
}

static void free_filter(Filter * filter) {
  switch(filter->ft) {
    case FNMATCH_FILTER:
      free(filter->info);
      break;
    case REG_FILTER:
      regfree(filter->info);
      free(filter->info);
      break;
    case TIME_FILTER:
      if (((struct time_info *)(filter->info))->tt == ANEWER 
          || ((struct time_info *)(filter->info))->tt == CNEWER
          || ((struct time_info *)(filter->info))->tt == MNEWER)
        free(((struct time_info *)(filter->info))->value);
      free(filter->info);
      break;
    default:
      //TODO
      break;
  }
  free(filter);
}

static void init_not_filter_adapter(Filter * filter) {
  filter->ft = NOT_FILTER_ADAPTER;
  filter->cmd = not_filter_adapter;
}

static void set_not_adapter_info (Filter * filter, Filter * inner_filter) {
  filter->info = inner_filter;
}

static bool not_filter_adapter(Filter * filter) {
  Filter * inner_filter = (Filter *) filter->info;
  return !execute_filter(inner_filter);
}


/**
 * regex filter
 */
static void init_reg(char * pattern) {
  int cflags = 0;
  Filter * filter = init_filter();
  filter->ft = REG_FILTER;
  filter->cmd = reg_filter;
  filter->info = malloc(sizeof(regex_t));
  regcomp(filter->info, pattern, cflags);
  push_op_stack(filter);
}

static bool reg_filter(Filter * filter) {
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

struct fn_option {
  bool case_s;
  char * pattern;
};

static void init_fnmatch(char * pattern, bool case_s) {
  Filter * filter = init_filter();  
  struct fn_option * fo = (struct fn_option *)malloc(sizeof(struct fn_option));
  fo->case_s = case_s;
  fo->pattern = pattern;
  filter->info = fo;
  filter->ft = FNMATCH_FILTER;
  filter->cmd = fnmatch_filter;
  push_op_stack(filter);
}

static bool fnmatch_filter(Filter * filter) {
  struct fn_option * fno = (struct fn_option *)filter->info;
  char * filename = cur_ent->fts_name;
  int ret;
  if (fno->case_s) {
    ret = fnmatch(fno->pattern, filename, FNM_PATHNAME|FNM_PERIOD);
  } else {
    ret = fnmatch(fno->pattern, filename, 
          FNM_PATHNAME|FNM_PERIOD|FNM_CASEFOLD);
  }
  if (ret == FNM_NOMATCH)
    return false;
  return true;
}

/* *
 * time filter
 * */

static void init_time(enum time_type tt, char * pattern) {
  Filter * filter = init_filter();  
  struct time_info * ti = malloc(sizeof(struct time_info));
  struct stat buf;
  ti->tt = tt;
  if (tt == ANEWER || tt == CNEWER || tt == MNEWER) {
    stat(pattern, &buf);
    time_t * time = (time_t *)malloc(sizeof(time_t)); 
    if (tt == ANEWER) {
      *time = buf.st_atime;
    } else if (tt == CNEWER) {
      *time = buf.st_ctime;
    } else {
      *time = buf.st_mtime;
    }
    ti->value = time;
  } else { 
    ti->value = pattern;
  }
  filter->info = ti;
  filter->ft = TIME_FILTER;
  filter->cmd = time_filter;
  push_op_stack(filter);
}

static bool time_filter(Filter * filter) {
  struct time_info * ti = filter->info;
  enum time_type tt = ti->tt;
  int t_del;

  time_t cur_time;
  double diff;

  bool result = false;
  
  if (tt == ANEWER || tt == CNEWER || tt == MNEWER) {
    cur_time = *(time_t *)ti->value;
  } else {
    t_del= atoi(ti->value);
    cur_time = time(NULL);
  }
  switch(tt) {
    case ATIME:
      diff = difftime(cur_time, status.st_atime);
      result = diff > t_del * 60 * 60 * 24;
      break;
    case AMIN:
      diff = difftime(cur_time, status.st_atime);
      result = diff > t_del * 60;
      break;
    case CTIME:
      diff = difftime(cur_time, status.st_ctime);
      result = diff > t_del * 60 * 60 * 24;
      break;
    case CMIN:
      diff = difftime(cur_time, status.st_ctime);
      result = diff > t_del * 60;
      break;
    case MTIME:
      diff = difftime(cur_time, status.st_mtime);
      result = diff > t_del * 60 * 60 * 24;
      break;
    case MMIN:
      diff = difftime(cur_time, status.st_mtime);
      result = diff > t_del * 60;
      break;
    case ANEWER:
      diff = difftime(status.st_atime, cur_time);
      result = diff >= 0.0;
      break;
    case CNEWER:
      diff = difftime(status.st_ctime, cur_time);
      result = diff >= 0.0;
      break;
    case MNEWER:
      diff = difftime(status.st_mtime, cur_time);
      result = diff >= 0.0;
      break;
  }
  return result;
}


static void init_filetype(char * ft) {
  Filter * filter = init_filter();
  filter->ft = FILETYPE_FILTER;
  filter->cmd = filetype_filter;
  filter->info = ft;
  push_op_stack(filter);
}

static bool filetype_filter(Filter * filter) {
  char * ft = (char *)filter->info;
  switch(ft[0]) {
    case 'd':
      return S_ISDIR(status.st_mode); 
    case 'c':
      return S_ISCHR(status.st_mode); 
    case 'b':
      return S_ISBLK(status.st_mode); 
    case 'p':
      return S_ISFIFO(status.st_mode); 
    case 'f':
      return S_ISREG(status.st_mode); 
    case 'l':
      return S_ISLNK(status.st_mode); 
    case 's':
      return S_ISSOCK(status.st_mode);
    default:
      break;
  }
  return false;
}

static void init_filesize(char * size) {
  Filter * filter = init_filter();
  filter->ft = FILESIZE_FILTER;
  filter->cmd = filesize_filter;
  filter->info = size;
  push_op_stack(filter);
}

static bool filesize_filter(Filter * filter) {
  char * str = (char *) (malloc(sizeof(char) * (strlen(filter->info) + 1)));
  char st = 0;
  strcpy(str, filter->info);
  bool result = false; 

  //int bn = status.st_blocks;
  //int bs = status.st_blksize;
  long all_size = status.st_size;
  long e_size;
  if (str[strlen(str)-1] > 58) {
    st = str[strlen(str) - 1];
    str[strlen(str) - 1] = '\0';
  }
  e_size = atoi(str);

  switch(st) {
    case 'P':
      e_size *= 1024;
    case 'T':
      e_size *= 1024;
    case 'G':
      e_size *= 1024;
    case 'M':
      e_size *= 1024;
    case 'k':
      e_size *= 1024;
      result = all_size == e_size;
      break;
    case 'c':
      result = all_size == e_size;
      break;
    default:
      all_size = ceil(all_size * 1.0 / 512);
      result = all_size == e_size; 
      break;
  }
  free(str);
  return result;
}

static bool is_int(char * item) {
  int i;
  for (i = 0; i < strlen(item); i++) {
    if (item[i] >= 58 || item[i] < 48)
      return false;
  }
  return true;
}

static void init_user(char * name) {
  Filter * filter = init_filter();
  filter->ft = USER_FILTER;
  filter->cmd = user_filter;
  filter->info = name;
  push_op_stack(filter);
}

static bool user_filter(Filter * filter) {
  char * info = (char *) filter->info;
  struct passwd * pwd = NULL;
  int user_id;
  if (is_int(info)) {
    user_id = atoi(info);
  } else {
    pwd = getpwnam(info);
    if (pwd == NULL) {
      return false;
    }
    user_id = pwd->pw_uid;
  }
  return user_id == status.st_uid;
}

static void init_group(char * name) {
  Filter * filter = init_filter();
  filter->ft = GROUP_FILTER;
  filter->cmd = group_filter;
  filter->info = name;
  push_op_stack(filter);
}

static bool group_filter(Filter * filter) {
  char * info = (char *) filter->info;
  struct group * grp = NULL;
  int group_id;
  if (is_int(info)) {
    group_id = atoi(info);
  } else {
    grp = getgrnam(info);
    if (grp == NULL) {
      return false;
    }
    group_id = grp->gr_gid;
  }
  return group_id == status.st_gid;
}

static void init_perm(char * name) {
  Filter * filter = init_filter();
  filter->ft = PERM_FILTER;
  filter->cmd = perm_filter;
  filter->info = name;
  push_op_stack(filter);
}

#define PERM_EQUAL(m, n) ((m & PERM_MASK) == n)

static bool perm_filter(Filter * filter) {
  unsigned long ul;
  char * tmp = (char *) malloc(sizeof(char) * (strlen(filter->info) + 2));
  tmp[0] = '0';
  strcpy(tmp + 1, filter->info);
  ul = strtoul (tmp,NULL,0);
  free(tmp);
  return PERM_EQUAL(status.st_mode, ul);
}


/* *
 * a filter that is always returns true
 * */
static bool true_filter(Filter * filter) { 
  return true; 
}
static void init_true() {
  Filter * filter = init_filter();
  filter->ft = TRUE_FILTER;
  filter->cmd = true_filter;
  push_op_stack(filter);
}
