#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

<<<<<<< HEAD
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
=======
static char ** post_exp;      // the postfix expression list
static int post_exp_index;    // the list size

static char ** help_stack;    // the helper stack to build the postfix expression
static int stack_index;       // the stack length

char * pop_stack();           // pop of the stack
char * top_stack();           // top of the stack
void push_stack(char * exp);  // push expression to stack
bool stack_empty();           // whether the stack is empty
void push_back_exp(char * exp);             //push exp to post_exp
void set_post_exp(int argc, char * argv[]); //build post_exp
>>>>>>> b1

char * get_exp() {
  static int i = -1;
  if (i == post_exp_index - 1) {
    return NULL;
  }
  i++;
  return post_exp[i];
}

<<<<<<< HEAD


=======
>>>>>>> b1
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
<<<<<<< HEAD
=======
  if (stack_index == 0) {
    return "";
  }
>>>>>>> b1
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
<<<<<<< HEAD
      pop_stack();
=======
      pop_stack(); //pop item until there is '('
      if (IS_EQUAL(top_stack(), "-not")) {
        char * tmp = post_exp[post_exp_index - 1];
        if (IS_NOT_EQUAL(tmp, "-and") && IS_NOT_EQUAL(tmp, "-or")) {
          push_back_exp("-and");
        }
        push_back_exp(pop_stack());
      } //test whether there is -not
>>>>>>> b1
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
<<<<<<< HEAD
      push_back_exp(exp);
    } else {
      push_back_exp(exp); 
=======
      push_stack(exp);
    } else {
      push_back_exp(exp); 
      if (IS_EQUAL(top_stack(), "-not") && ((i == argc - 1) || argv[i + 1][0] == '-')) {
        push_back_exp(pop_stack());
      }
>>>>>>> b1
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
