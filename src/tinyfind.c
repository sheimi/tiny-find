#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "filter.h"
#include "parser.h"

FindOption options;

static void find(char * path);
static void consider_check(FTS * fts, FTSENT * ent);
static void check(FTSENT * ent);
static void init_option();
static void free_option();
static void execute_exec(int argc, char ** argv, char * path, bool ask);

const char * USAGE = 
"usage: tinyfind [-H | -L | -P] [-EXdsx] [-f path] path ... [expression]\n"
"       tinyfind [-H | -L | -P] [-EXdsx] -f path [path ...] [expression]\n";

void print_error(const char * message) {
  if (message != NULL) {
    fputs(message, stderr);
  } else {
    fputs("encounter a error, exit\n", stderr);
  }
  exit(1);
}
void require_arg(const char * exp) {
  if (exp != NULL) {
    fprintf(stderr, "find: %s: requires additional arguments\n", exp);
    exit(1);
  } else {
    print_error("find: requires additional arguments\n");
  }
} 

int main(int argc, char * argv[]) {
  if (argc < 2) {
    print_error(USAGE);
  }

  init_option();
  //parse options and build filter tree
  init_parser(argc, argv);
  init_filter_tree(argc);
  free_parser();
  //walk the dir, excicute the filters and generate the result
  int i; 
  for (i = 0; i < options.pathes_num; i++) {
    find(options.find_pathes[0]);
  }
  
  //show the result
  //free filter
  free_filter_tree();
  free_option();
  return 0;
}

/* *
 * to walk the file hierarchy
 * */
static int fts_option = FTS_NOSTAT;

static void find(char * path) {
  char * arglist[2];
  FTS * fts;
  FTSENT * ent;

  arglist[0] = path;
  arglist[1] = NULL;
  
  //TODO ftsoption config

  switch (options.symbol_handle) {
    case S_L:
      fts_option |= FTS_COMFOLLOW|FTS_LOGICAL;
      break;
    case S_P:
      fts_option |= FTS_COMFOLLOW|FTS_PHYSICAL;
      break;
    case S_H:
      fts_option |= FTS_PHYSICAL;
      break;
  }

  fts = fts_open(arglist, fts_option, NULL);
  
  //TODO check fts

  //travel
  while ((ent = fts_read(fts)) != NULL) {
    consider_check(fts, ent);
  }
}

static void consider_check(FTS * fts, FTSENT * ent) {
  bool ignore = false;

  //ignore if the dir is accessed the second time
  if (ent->fts_info == FTS_DP) {
    ignore = true;
  }

  //max_depth
  if (options.max_depth >=0 && ent->fts_level >= options.max_depth) {
    fts_set(fts, ent, FTS_SKIP);
    if (ent->fts_level > options.max_depth)
      ignore = true;
  } else if (options.min_depth >=0 && ent->fts_level < options.min_depth) {
    ignore = true; 
  }

  if (!ignore) {
    check(ent);
  }

}
/* *
 *  to check whether the file satisfy the expressions
 * */
static void check(FTSENT * ent) {
  if (execute_filter_tree(ent)) {
    if (options.is_exec) {
      execute_exec(options.argc, options.argv, ent->fts_path, false);
    } 

    if (options.is_ok) {
      execute_exec(options.ok_argc, options.ok_argv, ent->fts_path, true);
    } 

    if (options.is_print0) {
      fprintf(stdout, "%s", ent->fts_path);
    }
    
    if (options.is_print || options.no_actions){
      fprintf(stdout, "%s\n", ent->fts_path);
    }
  }
}

static void init_option() {
  memset(&options, 0, sizeof(FindOption)); 
  options.max_depth = -1;
  options.min_depth = -1;
  options.no_actions = true;
}

static  void free_option() {
}

static void execute_exec(int argc, char ** argv, char * path, bool ask) {
  char ** argv_r = (char **)(malloc(sizeof(char *) * (argc)));
  int i = 0;
  for (i = 0; i < argc - 1; i++) {
    if (IS_EQUAL(argv[i], "{}")) {
      argv_r[i] = path;
    } else {
      argv_r[i] = argv[i];
    }
  }
  argv_r[argc - 1] = '\0';

  if (ask) {
    fputs("\"", stdout);
    for (i = 0; i < argc - 1; i++) {
      fprintf(stdout, "%s ", argv_r[i]);
    }
    fputs("\"?", stdout);
    char o = getchar();
    if (o != 'y' && o != 'Y') 
      return; 
  }

  pid_t pid;
  
  if ((pid = fork()) < 0) {
    fprintf(stderr, "ERROR\n");
  } else if (pid == 0) {
    execvp(argv_r[0], argv_r);
  } else {
    waitpid(pid, NULL, 0); 
  }

  free(argv_r);
}
