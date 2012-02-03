#include "filter.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


Filter * filter_tree;

struct stat status;


Filter * init_filter();
void free_filter_node(Filter * node); 
void free_filter_tree();
int exicute_filter(Filter * filter); 
bool exicute_filter_tree(char * path); 
bool true_filter(struct stat * status); 


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

bool exicute_filter_tree(char * path) {
  stat(path, &status); 
  bool passed = exicute_filter(filter_tree);
  return passed;
}

bool true_filter(struct stat * status) {
  return true;
}

void init_test() {
  filter_tree = init_filter();
  filter_tree->cmd = true_filter;
}


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
  free_filter_node(filter_tree); 
}
