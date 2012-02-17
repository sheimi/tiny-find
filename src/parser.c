#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

char * ALL_EXP[] = {
  "-name",
  "-iname",
  "-user",
  "-group",
  "-perm",
  "-regex",
  "-amin",
  "-atime",
  "-anewer",
  "-cmin",
  "-ctime",
  "-cnewer",
  "-mmin",
  "-mtime",
  "-mnewer",
  "-type",
  "-size",
};

char * OPTIONS[] = {
  "-P",
  "-H",
  "-L",
  "-exec",
  "-print0",
  "-print",
  "-depth",
  "-maxdepth",
  "-mindepth",
};

int ALL_EXP_LEN = 17;
int OPTIONS_LEN = 0;


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
bool is_logic_op(char * exp);
bool is_options(char * exp);

char * get_exp() {
  static int i = -1;
  if (i == post_exp_index - 1) {
    return NULL;
  }
  i++;
  return post_exp[i];
}

bool is_exp(char * arg) {
  int i;
  for (i = 0; i < ALL_EXP_LEN; i++) {
    if (IS_EQUAL(arg, ALL_EXP[i]))
      return true;
  } 
  return false;
}

bool is_options(char * arg) {
  int i;
  for (i = 0; i < OPTIONS_LEN; i++) {
    if (IS_EQUAL(arg, OPTIONS[i]))
      return true;
  } 
  return false;
}

bool is_logic_op(char * exp) {
  return IS_EQUAL(exp, "-and") || IS_EQUAL(exp, "-or") || IS_EQUAL(exp, "-not")
         || IS_EQUAL(exp, "(") || IS_EQUAL(exp, ")");
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
  if (stack_index == 0) {
    return "";
  }
  return help_stack[stack_index - 1];
}

bool stack_empty() {
  return stack_index == 0;
}

void set_post_exp(int argc, char * argv[]) {
  int i = 1;
  bool flag_c1 = false;
  bool flag_c2 = false;

  //parse options
  while (i < argc && argv[i][0] == '-') {
    char * exp = argv[i];
    i++;
    if (IS_EQUAL(exp, "-L")) {
      options.symbol_handle = S_L;
    } else if (IS_EQUAL(exp, "-P")) {
      options.symbol_handle = S_P;
    } else if (IS_EQUAL(exp, "-H")) {
      options.symbol_handle = S_H;
    } else if (IS_EQUAL(exp, "-f")) {
      break;
    }
  }


  //parse path
  if (i >= argc) {
    print_error(USAGE);
  }

  options.find_pathes = &argv[i];
  while (i < argc && argv[i][0] != '-') {
    options.pathes_num++;
    i++;
  }

  //parse actions and expressions
  for (; i < argc; i++) {
    char * exp = argv[i];
    if (IS_EQUAL(exp, "-depth")) {
      i++;
      options.max_depth = atoi(argv[i]);
      options.min_depth = atoi(argv[i]);
    } else if (IS_EQUAL(exp, "-maxdepth")) {
      i++;
      options.max_depth = atoi(argv[i]);
    } else if (IS_EQUAL(exp, "-mindepth")) {
      i++;
      options.min_depth = atoi(argv[i]);
    } else if (IS_EQUAL(exp, "-exec")) {
      int count = i;
      i++;
      options.is_exec = true;
      options.no_actions = false;
      options.argv = &argv[i];
      while (argv[i + 1][0] != ';') {
        i++;
      }
      options.argc = i - count + 1;
      i++;
    } else if (IS_EQUAL(exp, "-ok")) {
      int count = i;
      i++;
      options.is_ok= true;
      options.no_actions = false;
      options.ok_argv = &argv[i];
      while (argv[i + 1][0] != ';') {
        i++;
      }
      options.ok_argc = i - count + 1;
      i++;
    } else if (IS_EQUAL(exp, "-print")) {
      options.no_actions = false;
      options.is_print = true;
    } else if (IS_EQUAL(exp, "-print0")) {
      options.no_actions = false;
      options.is_print0 = true;
    } else if (IS_EQUAL(exp, "(")) {
      push_stack(exp);
    } else if (IS_EQUAL(exp, ")")) {
      while (!stack_empty() && IS_NOT_EQUAL(top_stack(), "(")) {
        push_back_exp(pop_stack());
      }
      pop_stack(); //pop item until there is '('
      if (IS_EQUAL(top_stack(), "-not")) {
        char * tmp = post_exp[post_exp_index - 1];
        if (IS_NOT_EQUAL(tmp, "-and") && IS_NOT_EQUAL(tmp, "-or")) {
          push_back_exp("-and");
        }
        push_back_exp(pop_stack());
      } //test whether there is -not
    } else if (IS_EQUAL(exp, "-and")) {
      if (!stack_empty() && IS_EQUAL(top_stack(), "-and")) {
        push_back_exp(exp);
      } else {
        push_stack(exp);
      }
      flag_c1 = false;
    } else if (IS_EQUAL(exp, "-or")) {
      while (!stack_empty() && IS_NOT_EQUAL(top_stack(), "(")) {
        push_back_exp(pop_stack());
      }
      push_stack(exp);
      flag_c1 = false;
    } else if (IS_EQUAL(exp, "-not")) {
      push_stack(exp);
    } else if (is_exp(exp)) {
      push_back_exp(exp); 
      if (flag_c1) {
        flag_c2 = true;
      }
      flag_c1 = true;
    } else {
      push_back_exp(exp); 
      if (IS_EQUAL(top_stack(), "-not")) {
        push_back_exp(pop_stack());
      }
      if (flag_c2) {
        push_back_exp("-and");
        flag_c2 = false;
      }
    }
  }

  while (!stack_empty()) {
    push_back_exp(pop_stack());
  }

#ifdef DEBUG
  fprintf(stderr, "\n");
  for (i = 0; i < post_exp_index; i++) {
    fprintf(stderr, "%s\n", post_exp[i]);
  }
  fprintf(stderr, "\n");
#endif
}

void free_parser() {
  free(post_exp);
  free(help_stack);
}
