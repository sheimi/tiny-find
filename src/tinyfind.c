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
static void execute_exec(int argc, char ** argv, char * path);

const char * USAGE = 
"usage: tinyfind [-H | -L | -P] [-EXdsx] [-f path] path ... [expression]\n"
"       tinyfind [-H | -L | -P] [-EXdsx] -f path [path ...] [expression]\n";

void print_error(const char * message) {
  if (message != NULL) {
    fputs(message, stderr);
  } else {
    fputs("encounter a error, exit", stderr);
  }
  exit(1);
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
  
  find(options.find_dir);
  
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
      execute_exec(options.argc, options.argv, ent->fts_path);
    } else {
      fprintf(stdout, "%s\n", ent->fts_path);
    }
  }
}

static void init_option() {
  memset(&options, 0, sizeof(FindOption)); 
  options.max_depth = -1;
  options.min_depth = -1;
}

static  void free_option() {
}

static void execute_exec(int argc, char ** argv, char * path) {
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

  pid_t pid;
  
  if ((pid = fork()) < 0) {
    fprintf(stderr, "ERROR");
  } else if (pid == 0) {
    execvp(argv_r[0], argv_r);
  } else {
    waitpid(pid, NULL, 0); 
  }

  free(argv_r);
}
