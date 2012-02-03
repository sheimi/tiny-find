#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "find.h"

static int fts_option = FTS_NOSTAT;
static void find(char * path);
static void consider_check(FTSENT * ent);
static void check(char * path);

int main(int argc, char * argv[]) {
  //parse options and build filter tree
  init_test();
  //walk the dir, excicute the filters and generate the result
  
  char * dir = ".";
  find(dir);
  
  //show the result
  //free filter
  free_filter_tree();
}

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

  if (ent->fts_info == FTS_DP) {
    ignore = true;
  }

  if (!ignore) {
    check(ent->fts_path);
  }

}

static void check(char * path) {
  fprintf(stderr, "%d\n", exicute_filter_tree(path));
}
