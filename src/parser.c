#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

static char ** post_exp;
static int post_exp_index;

static char ** help_stack;
static int stack_index;

void init_parser(int argc, char * argv[]);
void free_parser();
char * pop_stack();
char * top_stack();
void push_stack(char * exp);
bool stack_empty();
void push_back_exp(char * exp);
void set_post_exp(int argc, char * argv[]);
char * get_exp();

char * get_exp() {
  static int i = -1;
  if (i == post_exp_index - 1) {
    return NULL;
  }
  i++;
  return post_exp[i];
}



void init_parser(int argc, char * argv[]) {
  post_exp = (char **)(malloc(sizeof(char **) * argc));
  help_stack = (char **)(malloc(sizeof(char **) * argc));
  post_exp_index = 0;
  stack_index = 0;
  set_post_exp(argc, argv);
}

void push_back_exp(char * exp) {
  post_exp[post_exp_index] = exp;
  post_exp_index++;
}

void push_stack(char * exp) {
  help_stack[stack_index] = exp;
  stack_index++;
}

char * pop_stack() {
  stack_index--; 
  return help_stack[stack_index];
}

char * top_stack() {
  return help_stack[stack_index - 1];
}

bool stack_empty() {
  return stack_index == 0;
}

void set_post_exp(int argc, char * argv[]) {
  int i;
  for (i = 2; i < argc; i++) {
    char * exp = argv[i];
    if (IS_EQUAL(exp, "(")) {
      push_stack(exp);
    } else if (IS_EQUAL(exp, ")")) {
      while (!stack_empty() && IS_NOT_EQUAL(top_stack(), "(")) {
        push_back_exp(pop_stack());
      }
      pop_stack();
    } else if (IS_EQUAL(exp, "-and")) {
      if (!stack_empty() && IS_EQUAL(top_stack(), "-and")) {
        push_back_exp(exp);
      } else {
        push_stack(exp);
      }
    } else if (IS_EQUAL(exp, "-or")) {
      while (!stack_empty() && IS_NOT_EQUAL(top_stack(), "(")) {
        push_back_exp(pop_stack());
      }
      push_stack(exp);
    } else if (IS_EQUAL(exp, "-not")) {
      push_back_exp(exp);
    } else {
      push_back_exp(exp); 
    }
  }

  while (!stack_empty()) {
    push_back_exp(pop_stack());
  }

  fprintf(stderr, "\n\n");
  for (i = 0; i < post_exp_index; i++) {
    fprintf(stderr, "%s\n", post_exp[i]);
  }
  fprintf(stderr, "\n\n");
}

void free_parser() {
  free(post_exp);
  free(help_stack);
}
