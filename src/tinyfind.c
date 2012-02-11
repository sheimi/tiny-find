#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "env.h"
#include "filter.h"
#include "parser.h"

static int fts_option = FTS_NOSTAT;
static void find(char * path);
static void consider_check(FTSENT * ent);
static void check(FTSENT * ent);

int main(int argc, char * argv[]) {
  char * dir = argv[1];

  //parse options and build filter tree
  init_filter_tree(argc, argv);
  //walk the dir, excicute the filters and generate the result
  
  find(dir);
  
  //show the result
  //free filter
  free_filter_tree();
  return 0;
}

/* *
 * to walk the file hierarchy
 * */
static void find(char * path) {
  char * arglist[2];
  FTS * fts;
  FTSENT * ent;

  arglist[0] = path;
  arglist[1] = NULL;
  
  //TODO ftsoption config

  fts = fts_open(arglist, fts_option, NULL);
  
  //TODO check fts

  //travel
  while ((ent = fts_read(fts)) != NULL) {
    consider_check(ent);
  }
}

static void consider_check(FTSENT * ent) {
  bool ignore = false;

  //ignore if the dir is accessed the second time
  if (ent->fts_info == FTS_DP) {
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
    fprintf(stdout, "%s\n", ent->fts_path);
  }
}
