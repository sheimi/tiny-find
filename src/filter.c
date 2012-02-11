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
#include <string.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>

static Filter filter_tree;

struct stat status;
FTSENT * cur_ent;

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
Filter * init_filter();
int execute_filter(Filter * filter); 
void init_visit_filter();

void connect_filter(Filter * first, Filter * second);
void filter_and();
void filter_or(); 
void filter_not();

void init_not_filter_adapter(Filter * filter);
bool not_filter_adapter(Filter * filter);
void set_not_adapter_info (Filter * filter, Filter * inner_filter); 

//filters
void init_reg(char * pattern);
bool reg_filter(Filter * filter);

void init_fnmatch(char * pattern, bool case_s); 
bool fnmatch_filter(Filter * filter); 

void init_time(enum time_type tt, char * pattern);
bool time_filter(Filter * filter);

void init_filetype(char ft);
bool filetype_filter(Filter * filter);

void init_filesize(char * size);
bool filesize_filter(Filter * filter);

void init_user(char * name);
bool user_filter(Filter * filter);

void init_group(char * name);
bool group_filter(Filter * filter);

void init_perm(char * name);
bool perm_filter(Filter * filter);

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
      init_fnmatch(optarg, true);
    } else if (IS_EQUAL(exp, "-iname")) {
      optarg = get_exp();
      fprintf(stderr, "filename is %s\n", optarg);
      init_fnmatch(optarg, false);
    } else if (IS_EQUAL(exp, "-user")) {
      optarg = get_exp();
      fprintf(stderr, "user is %s\n", optarg);
      init_user(optarg);
    } else if (IS_EQUAL(exp, "-group")) {
      optarg = get_exp();
      fprintf(stderr, "group is %s\n", optarg);
      init_group(optarg);
    } else if (IS_EQUAL(exp, "-perm")) {
      optarg = get_exp();
      fprintf(stderr, "perm is %s\n", optarg);
      init_perm(optarg);
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
    } else if (IS_EQUAL(exp, "-anewer")) {
      optarg = get_exp();
      fprintf(stderr, "test file is %s\n", optarg);
      init_time(ANEWER, optarg);
    } else if (IS_EQUAL(exp, "-cnewer")) {
      optarg = get_exp();
      fprintf(stderr, "test file is %s\n", optarg);
      init_time(CNEWER, optarg);
    } else if (IS_EQUAL(exp, "-cmin")) {
      optarg = get_exp();
      fprintf(stderr, "min deltais %s\n", optarg);
      init_time(CMIN, optarg);
    } else if (IS_EQUAL(exp, "-ctime")) {
      optarg = get_exp();
      fprintf(stderr, "day delta is %s\n", optarg);
      init_time(CTIME, optarg);
    } else if (IS_EQUAL(exp, "-mtime")) {
      optarg = get_exp();
      fprintf(stderr, "day delta is %s\n", optarg);
      init_time(MTIME, optarg);
    } else if (IS_EQUAL(exp, "-mmin")) {
      optarg = get_exp();
      fprintf(stderr, "min delta is %s\n", optarg);
      init_time(MMIN, optarg);
    } else if (IS_EQUAL(exp, "-mnewer")) {
      optarg = get_exp();
      fprintf(stderr, "test file is %s\n", optarg);
      init_time(MNEWER, optarg);
    } else if (IS_EQUAL(exp, "-type")) {
      optarg = get_exp();
      fprintf(stderr, "file type is %s\n", optarg);
      init_filetype(optarg[0]);
    } else if (IS_EQUAL(exp, "-size")) {
      optarg = get_exp();
      fprintf(stderr, "file size is %s\n", optarg);
      init_filesize(optarg);
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

struct fn_option {
  bool case_s;
  char * pattern;
};

void init_fnmatch(char * pattern, bool case_s) {
  Filter * filter = init_filter();  
  struct fn_option * fo = (struct fn_option *)malloc(sizeof(struct fn_option));
  fo->case_s = case_s;
  fo->pattern = pattern;
  filter->info = fo;
  filter->ft = FNMATCH_FILTER;
  filter->cmd = fnmatch_filter;
  push_op_stack(filter);
}

bool fnmatch_filter(Filter * filter) {
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

void init_time(enum time_type tt, char * pattern) {
  Filter * filter = init_filter();  
  struct time_info * ti = malloc(sizeof(struct time_info));
  struct stat buf;
  ti->tt = tt;
  if (tt == ANEWER || tt == CNEWER || tt == MNEWER) {
    stat(pattern, &buf);
    time_t * time = (time_t)malloc(sizeof(time_t)); 
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

bool time_filter(Filter * filter) {
  struct time_info * ti = filter->info;
  enum time_type tt = ti->tt;
  int t_del;

  struct stat buf;
  time_t cur_time;
  double diff;

  bool result = false;
  
  if (tt == ANEWER || tt == CNEWER) {
    cur_time = *(time_t *)ti->value;
  } else {
    t_del= atoi(ti->value);
    cur_time = time(NULL);
  }
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
    case MTIME:
      diff = difftime(cur_time, buf.st_mtime);
      result = diff < t_del * 60 * 60 * 24;
      break;
    case MMIN:
      diff = difftime(cur_time, buf.st_mtime);
      result = diff < t_del * 60;
      break;
    case ANEWER:
      diff = difftime(buf.st_atime, cur_time);
      result = diff > 0;
      break;
    case CNEWER:
      diff = difftime(buf.st_ctime, cur_time);
      result = diff > 0;
      break;
    case MNEWER:
      diff = difftime(buf.st_mtime, cur_time);
      result = diff > 0;
      break;
  }
  return result;
}


void init_filetype(char ft) {
  Filter * filter = init_filter();
  filter->ft = FILETYPE_FILTER;
  filter->cmd = filetype_filter;
  filter->info = ft;
  push_op_stack(filter);
}

bool filetype_filter(Filter * filter) {
  struct stat buf;
  stat(cur_ent->fts_path, &buf);
  char ft = (char)filter->info;
  switch(ft) {
    case 'd':
      return S_ISDIR(buf.st_mode); 
    case 'c':
      return S_ISCHR(buf.st_mode); 
    case 'b':
      return S_ISBLK(buf.st_mode); 
    case 'p':
      return S_ISFIFO(buf.st_mode); 
    case 'f':
      return S_ISREG(buf.st_mode); 
    case 'l':
      return S_ISLNK(buf.st_mode); 
    case 's':
      //TODO to set is a socket
      break;
    default:
      break;
  }
  return false;
}
void init_filesize(char * size) {
  Filter * filter = init_filter();
  filter->ft = FILESIZE_FILTER;
  filter->cmd = filesize_filter;
  filter->info = size;
  push_op_stack(filter);
}

bool filesize_filter(Filter * filter) {
  char * str = (char *) (malloc(sizeof(char) * (strlen(filter->info) + 1)));
  char st = 0;
  strcpy(str, filter->info);
  struct stat buf;
  stat(cur_ent->fts_path, &buf);
  bool result = false; 

  int bn = buf.st_blocks;
  int bs = buf.st_blksize;
  long all_size = buf.st_size;
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

bool is_int(char * item) {
  int i;
  for (i = 0; i < strlen(item); i++) {
    if (item[i] >= 58 || item[i] < 48)
      return false;
  }
  return true;
}

void init_user(char * name) {
  Filter * filter = init_filter();
  filter->ft = USER_FILTER;
  filter->cmd = user_filter;
  filter->info = name;
  push_op_stack(filter);
}

bool user_filter(Filter * filter) {
  char * info = (char *) filter->info;
  struct passwd * pwd = NULL;
  int user_id;
  struct stat buf;
  if (is_int(info)) {
    user_id = atoi(info);
  } else {
    pwd = getpwnam(info);
    if (pwd == NULL) {
      return false;
    }
    user_id = pwd->pw_uid;
  }
  stat(cur_ent->fts_path, &buf);
  return user_id == buf.st_uid;
}

void init_group(char * name) {
  Filter * filter = init_filter();
  filter->ft = GROUP_FILTER;
  filter->cmd = group_filter;
  filter->info = name;
  push_op_stack(filter);
}

bool group_filter(Filter * filter) {
  char * info = (char *) filter->info;
  struct group * grp = NULL;
  int group_id;
  struct stat buf;
  if (is_int(info)) {
    grp = atoi(info);
  } else {
    grp = getgrnam(info);
    if (grp == NULL) {
      return false;
    }
    group_id = grp->gr_gid;
  }
  stat(cur_ent->fts_path, &buf);
  return group_id == buf.st_gid;
}

void init_perm(char * name) {
  Filter * filter = init_filter();
  filter->ft = PERM_FILTER;
  filter->cmd = perm_filter;
  filter->info = name;
  push_op_stack(filter);
}

#define PERM_EQUAL(m, n) ((m & PERM_MASK) == n)

bool perm_filter(Filter * filter) {
  unsigned long ul;
  struct stat buf;
  char * tmp = (char *) malloc(sizeof(char) * (strlen(filter->info) + 2));
  tmp[0] = '0';
  strcpy(tmp + 1, filter->info);
  ul = strtoul (tmp,NULL,0);
  stat(cur_ent->fts_path, &buf);
  return PERM_EQUAL(buf.st_mode, ul);
}
